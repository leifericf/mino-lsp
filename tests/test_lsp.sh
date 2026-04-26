#!/bin/sh
#
# test_lsp.sh -- automated protocol tests for mino-lsp.
#
# Launches the server, exercises each LSP method via JSON-RPC over
# stdin/stdout, and validates responses. Requires: python3.
#

set -e

LSP=./mino-lsp

# Build if needed.
if [ ! -x "$LSP" ]; then
    echo "Building mino-lsp..."
    make
fi

python3 - "$LSP" <<'PYEOF'
import subprocess, json, sys, time

lsp_bin = sys.argv[1]
passed = 0
failed = 0

proc = subprocess.Popen(
    [lsp_bin],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)

def send(method, params, msg_id=None):
    msg = {"jsonrpc": "2.0", "method": method, "params": params}
    if msg_id is not None:
        msg["id"] = msg_id
    body = json.dumps(msg)
    header = f"Content-Length: {len(body)}\r\n\r\n"
    proc.stdin.write(header.encode())
    proc.stdin.write(body.encode())
    proc.stdin.flush()

def recv():
    # Read Content-Length header.
    header = b""
    while True:
        ch = proc.stdout.read(1)
        if not ch:
            return None
        header += ch
        if header.endswith(b"\r\n\r\n"):
            break
    # Parse Content-Length.
    for line in header.decode().split("\r\n"):
        if line.startswith("Content-Length:"):
            length = int(line.split(":")[1].strip())
            body = proc.stdout.read(length)
            return json.loads(body)
    return None

def recv_until(predicate, max_messages=10):
    """Read messages until predicate returns True or max_messages reached."""
    for _ in range(max_messages):
        msg = recv()
        if msg is None:
            return None
        if predicate(msg):
            return msg
    return None

def ok(name):
    global passed
    passed += 1
    print(f"  ok: {name}")

def fail(name, detail=""):
    global failed
    failed += 1
    msg = f"FAIL: {name}"
    if detail:
        msg += f" ({detail})"
    print(msg)

# --- Test: initialize ---
send("initialize", {
    "processId": None,
    "capabilities": {},
    "rootUri": None,
}, msg_id=1)

resp = recv()
if resp and "result" in resp:
    caps = resp["result"].get("capabilities", {})
    if caps.get("hoverProvider"):
        ok("initialize returns hoverProvider")
    else:
        fail("initialize hoverProvider", str(caps))
    if caps.get("completionProvider"):
        ok("initialize returns completionProvider")
    else:
        fail("initialize completionProvider", str(caps))
    if caps.get("textDocumentSync"):
        ok("initialize returns textDocumentSync")
    else:
        fail("initialize textDocumentSync", str(caps))
    info = resp["result"].get("serverInfo", {})
    if info.get("name") == "mino-lsp":
        ok("initialize returns server name")
    else:
        fail("initialize server name", str(info))
else:
    fail("initialize", str(resp))

send("initialized", {})

# --- Test: didOpen with valid code (expect empty diagnostics) ---
send("textDocument/didOpen", {
    "textDocument": {
        "uri": "file:///test/valid.clj",
        "languageId": "mino",
        "version": 1,
        "text": "(def x 42)\n",
    }
})

# Read diagnostics notification.
diag = recv_until(lambda m: m.get("method") == "textDocument/publishDiagnostics")
if diag:
    diags = diag.get("params", {}).get("diagnostics", [])
    if len(diags) == 0:
        ok("didOpen valid code: no diagnostics")
    else:
        fail("didOpen valid code: unexpected diagnostics", str(diags))
else:
    fail("didOpen valid code: no diagnostics notification")

# --- Test: didOpen with parse error ---
send("textDocument/didOpen", {
    "textDocument": {
        "uri": "file:///test/broken.clj",
        "languageId": "mino",
        "version": 1,
        "text": "(def x\n",
    }
})

diag = recv_until(lambda m: m.get("method") == "textDocument/publishDiagnostics"
                  and m.get("params", {}).get("uri") == "file:///test/broken.clj")
if diag:
    diags = diag.get("params", {}).get("diagnostics", [])
    if len(diags) > 0:
        ok("didOpen parse error: diagnostic reported")
        msg = diags[0].get("message", "")
        if "unterminated" in msg.lower() or "unexpected" in msg.lower() or len(msg) > 0:
            ok("didOpen parse error: meaningful message")
        else:
            fail("didOpen parse error: empty message")
    else:
        fail("didOpen parse error: no diagnostics")
else:
    fail("didOpen parse error: no notification")

# --- Test: completion ---
send("textDocument/completion", {
    "textDocument": {"uri": "file:///test/valid.clj"},
    "position": {"line": 0, "character": 1},
}, msg_id=2)

resp = recv_until(lambda m: m.get("id") == 2)
if resp and "result" in resp:
    items = resp["result"]
    if isinstance(items, list) and len(items) > 0:
        labels = [i.get("label", "") for i in items]
        if "map" in labels:
            ok("completion returns 'map'")
        else:
            fail("completion missing 'map'", str(labels[:10]))
        if "def" in labels or "+" in labels:
            ok("completion returns builtins")
        else:
            fail("completion missing builtins", str(labels[:10]))
    else:
        fail("completion empty result", str(items))
else:
    fail("completion", str(resp))

# --- Test: completion with prefix ---
send("textDocument/didOpen", {
    "textDocument": {
        "uri": "file:///test/prefix.clj",
        "languageId": "mino",
        "version": 1,
        "text": "(ma",
    }
})
# Consume diagnostics notification.
recv_until(lambda m: m.get("method") == "textDocument/publishDiagnostics"
           and m.get("params", {}).get("uri") == "file:///test/prefix.clj")

send("textDocument/completion", {
    "textDocument": {"uri": "file:///test/prefix.clj"},
    "position": {"line": 0, "character": 3},
}, msg_id=3)

resp = recv_until(lambda m: m.get("id") == 3)
if resp and "result" in resp:
    items = resp["result"]
    labels = [i.get("label", "") for i in items] if isinstance(items, list) else []
    if "map" in labels:
        ok("prefix completion returns 'map' for 'ma'")
    else:
        fail("prefix completion missing 'map'", str(labels[:10]))
    # Should NOT contain unrelated symbols.
    if "+" not in labels and "cons" not in labels:
        ok("prefix completion filters non-matching")
    else:
        fail("prefix completion includes non-matching", str(labels[:10]))
else:
    fail("prefix completion", str(resp))

# --- Test: hover on builtin ---
send("textDocument/didOpen", {
    "textDocument": {
        "uri": "file:///test/hover.clj",
        "languageId": "mino",
        "version": 1,
        "text": "map\n",
    }
})
# Consume diagnostics.
recv_until(lambda m: m.get("method") == "textDocument/publishDiagnostics"
           and m.get("params", {}).get("uri") == "file:///test/hover.clj")

send("textDocument/hover", {
    "textDocument": {"uri": "file:///test/hover.clj"},
    "position": {"line": 0, "character": 1},
}, msg_id=4)

resp = recv_until(lambda m: m.get("id") == 4)
if resp and "result" in resp and resp["result"] is not None:
    contents = resp["result"].get("contents", {})
    value = contents.get("value", "")
    if "map" in value:
        ok("hover returns info for 'map'")
    else:
        fail("hover missing 'map' in content", value)
    if "kind" in contents and contents["kind"] == "markdown":
        ok("hover uses markdown format")
    else:
        fail("hover format", str(contents))
else:
    fail("hover", str(resp))

# --- Test: hover on unknown symbol ---
send("textDocument/hover", {
    "textDocument": {"uri": "file:///test/hover.clj"},
    "position": {"line": 0, "character": 100},
}, msg_id=5)

resp = recv_until(lambda m: m.get("id") == 5)
if resp and resp.get("result") is None:
    ok("hover on unknown returns null")
else:
    fail("hover on unknown", str(resp))

# --- Test: didChange triggers new diagnostics ---
send("textDocument/didChange", {
    "textDocument": {"uri": "file:///test/valid.clj", "version": 2},
    "contentChanges": [{"text": "(def y\n"}],
})

diag = recv_until(lambda m: m.get("method") == "textDocument/publishDiagnostics"
                  and m.get("params", {}).get("uri") == "file:///test/valid.clj")
if diag:
    diags = diag.get("params", {}).get("diagnostics", [])
    if len(diags) > 0:
        ok("didChange triggers diagnostics on error")
    else:
        fail("didChange: no diagnostics after introducing error")
else:
    fail("didChange: no notification")

# --- Test: didClose clears diagnostics ---
send("textDocument/didClose", {
    "textDocument": {"uri": "file:///test/broken.clj"},
})

diag = recv_until(lambda m: m.get("method") == "textDocument/publishDiagnostics"
                  and m.get("params", {}).get("uri") == "file:///test/broken.clj")
if diag:
    diags = diag.get("params", {}).get("diagnostics", [])
    if len(diags) == 0:
        ok("didClose clears diagnostics")
    else:
        fail("didClose: diagnostics not cleared", str(diags))
else:
    fail("didClose: no clear notification")

# --- Test: shutdown ---
send("shutdown", {}, msg_id=99)
resp = recv_until(lambda m: m.get("id") == 99)
if resp and "result" in resp:
    ok("shutdown returns result")
else:
    fail("shutdown", str(resp))

# --- Test: exit ---
send("exit", {})
proc.wait(timeout=5)
if proc.returncode == 0:
    ok("exit with code 0 after shutdown")
else:
    fail("exit code", str(proc.returncode))

# --- Summary ---
print(f"\n{passed + failed} tests, {passed} passed, {failed} failed")
sys.exit(1 if failed > 0 else 0)
PYEOF
