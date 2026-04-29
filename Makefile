CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wpedantic -Wextra -O2
MINO_INCS = -Imino/src -Imino/src/public -Imino/src/runtime \
            -Imino/src/gc -Imino/src/eval -Imino/src/collections \
            -Imino/src/prim -Imino/src/async -Imino/src/interop \
            -Imino/src/diag -Imino/src/vendor/imath
MINO_SRCS = $(wildcard mino/src/public/*.c) \
            $(wildcard mino/src/runtime/*.c) \
            $(wildcard mino/src/gc/*.c) \
            $(wildcard mino/src/eval/*.c) \
            $(wildcard mino/src/collections/*.c) \
            $(wildcard mino/src/prim/*.c) \
            $(wildcard mino/src/async/*.c) \
            $(wildcard mino/src/interop/*.c) \
            $(wildcard mino/src/regex/*.c) \
            $(wildcard mino/src/diag/*.c) \
            $(wildcard mino/src/vendor/imath/*.c)
SRCS     = src/main.c src/json.c src/lsp.c src/document.c src/diagnostic.c $(MINO_SRCS)
TARGET   = mino-lsp

.DEFAULT_GOAL := $(TARGET)

# --- Bundled-stdlib generated headers ---
# install_stdlib.c #includes one C string-literal header per bundled
# namespace; these are gitignored generated artifacts.

define gen-mino-header
mino/src/$(2).h: mino/$(1)
	@printf 'static const char *$(2)_src =\n' > $$@
	@sed 's/\\/\\\\/g; s/"/\\"/g; s/^/    "/; s/$$$$/\\n"/' $$< >> $$@
	@printf '    ;\n' >> $$@
endef

$(eval $(call gen-mino-header,src/core.clj,core_mino))
$(eval $(call gen-mino-header,lib/clojure/string.clj,lib_clojure_string))
$(eval $(call gen-mino-header,lib/clojure/set.clj,lib_clojure_set))
$(eval $(call gen-mino-header,lib/clojure/walk.clj,lib_clojure_walk))
$(eval $(call gen-mino-header,lib/clojure/edn.clj,lib_clojure_edn))
$(eval $(call gen-mino-header,lib/clojure/pprint.clj,lib_clojure_pprint))
$(eval $(call gen-mino-header,lib/clojure/zip.clj,lib_clojure_zip))
$(eval $(call gen-mino-header,lib/clojure/data.clj,lib_clojure_data))
$(eval $(call gen-mino-header,lib/clojure/test.clj,lib_clojure_test))
$(eval $(call gen-mino-header,lib/clojure/template.clj,lib_clojure_template))
$(eval $(call gen-mino-header,lib/clojure/repl.clj,lib_clojure_repl))
$(eval $(call gen-mino-header,lib/clojure/stacktrace.clj,lib_clojure_stacktrace))
$(eval $(call gen-mino-header,lib/clojure/datafy.clj,lib_clojure_datafy))
$(eval $(call gen-mino-header,lib/clojure/core/protocols.clj,lib_clojure_core_protocols))
$(eval $(call gen-mino-header,lib/clojure/instant.clj,lib_clojure_instant))
$(eval $(call gen-mino-header,lib/clojure/spec/alpha.clj,lib_clojure_spec_alpha))
$(eval $(call gen-mino-header,lib/clojure/core/specs/alpha.clj,lib_clojure_core_specs_alpha))
$(eval $(call gen-mino-header,lib/mino/deps.clj,lib_mino_deps))
$(eval $(call gen-mino-header,lib/mino/tasks.clj,lib_mino_tasks))
$(eval $(call gen-mino-header,lib/mino/tasks/builtin.clj,lib_mino_tasks_builtin))

MINO_GEN_HEADERS = mino/src/core_mino.h \
                   mino/src/lib_clojure_string.h \
                   mino/src/lib_clojure_set.h \
                   mino/src/lib_clojure_walk.h \
                   mino/src/lib_clojure_edn.h \
                   mino/src/lib_clojure_pprint.h \
                   mino/src/lib_clojure_zip.h \
                   mino/src/lib_clojure_data.h \
                   mino/src/lib_clojure_test.h \
                   mino/src/lib_clojure_template.h \
                   mino/src/lib_clojure_repl.h \
                   mino/src/lib_clojure_stacktrace.h \
                   mino/src/lib_clojure_datafy.h \
                   mino/src/lib_clojure_core_protocols.h \
                   mino/src/lib_clojure_instant.h \
                   mino/src/lib_clojure_spec_alpha.h \
                   mino/src/lib_clojure_core_specs_alpha.h \
                   mino/src/lib_mino_deps.h \
                   mino/src/lib_mino_tasks.h \
                   mino/src/lib_mino_tasks_builtin.h

$(TARGET): $(SRCS) src/json.h src/lsp.h src/document.h src/diagnostic.h \
           mino/src/mino.h mino/src/diag/diag.h $(MINO_GEN_HEADERS)
	$(CC) $(CFLAGS) $(MINO_INCS) -Isrc -o $@ $(SRCS) -lm

test: $(TARGET)
	tests/test_lsp.sh

clean:
	rm -f $(TARGET) $(MINO_GEN_HEADERS)

.PHONY: test clean
