# mino-lsp

A [Language Server Protocol](https://microsoft.github.io/language-server-protocol/) server for [mino](https://github.com/leifericf/mino). Provides real-time diagnostics, symbol completion, and hover documentation in any editor that speaks LSP.

Written in C99 with no dependencies beyond mino itself.

## Features

| Feature | Description |
|---------|-------------|
| Diagnostics | Parse and eval errors shown inline as you type |
| Completion | Symbol suggestions filtered by prefix |
| Hover | Type and docstring for the symbol under the cursor |

## Build

```
git clone --recursive https://github.com/leifericf/mino-lsp.git
cd mino-lsp
make
```

This produces a single `mino-lsp` binary.

## Usage

mino-lsp communicates over stdin/stdout using JSON-RPC 2.0. Editors launch it as a subprocess and send LSP messages.

## Editor Setup

### Neovim

Add a custom LSP server configuration:

```lua
local configs = require("lspconfig.configs")
configs.mino = {
  default_config = {
    cmd = { "mino-lsp" },
    filetypes = { "mino" },
    root_dir = function(fname)
      return vim.fs.dirname(fname)
    end,
  },
}
require("lspconfig").mino.setup({})
```

### Helix

Add to `~/.config/helix/languages.toml`:

```toml
[language-server.mino-lsp]
command = "mino-lsp"

[[language]]
name = "mino"
language-servers = ["mino-lsp"]
```

### Emacs

With eglot (built into Emacs 29+):

```elisp
(add-to-list 'eglot-server-programs '(mino-mode "mino-lsp"))
```

### VS Code

Install a generic LSP client extension and point it at the `mino-lsp` binary.

### Zed

Add `mino-lsp` as the language server in a mino language extension.

## How It Works

mino-lsp embeds the mino runtime. When you open or edit a file, it parses the content with `mino_read()` and evaluates it with `mino_eval()` to find both syntax errors and semantic errors. Completions use `(apropos "")` to enumerate all bound symbols. Hover uses `(doc sym)` for docstrings.

The server runs single-threaded with a step limit on evaluation to prevent infinite loops in user code. The LSP environment does not install I/O primitives, so evaluated code cannot produce side effects.

## Tests

```
make test
```

Runs the protocol test suite (requires Python 3).

## License

MIT
