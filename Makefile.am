# Copyright © 2005 Adam Jackson.
# Copyright © 2009 Intel Corporation
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = lib man tools scripts benchmarks demos overlay

if BUILD_ASSEMBLER
SUBDIRS += assembler
endif

if BUILD_SHADER_DEBUGGER
SUBDIRS += debugger
endif

if BUILD_TESTS
SUBDIRS += piglit tests
endif

MAINTAINERCLEANFILES = ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

#
# Useful shortcuts to run tests
#
PIGLIT := $(srcdir)/piglit/piglit-run.py --no-concurrency
PIGLIT_SUMMARY := $(srcdir)/piglit/piglit-summary.py -s
PIGLIT_HTML := $(srcdir)/piglit/piglit-summary-html.py
IGT_TESTS := $(abs_srcdir)/piglit/tests/igt.tests
TEST_TARGETS := #

TEST_TARGETS += run-test
run-tests:
	@if [ -n "$(RESUME)" ]; then \
		[ ! -f $(RESUME)/main ] && { \
			echo "$(RESUME) is not a valid piglit project"; \
			exit 1; \
		}; \
		output=$(RESUME); \
		echo "Resuming $(RESUME)..."; \
		PIGLIT_FLAGS="--resume $$PIGLIT_FLAGS"; \
	else \
		source $(srcdir)/scripts/build-piglit.sh && \
		output=$$(igt_result_directory); \
	fi; \
	$(PIGLIT) $${PIGLIT_FLAGS} $(IGT_TESTS) $$output && \
	$(PIGLIT_HTML) --overwrite $$output/html $$output/main && \
	$(PIGLIT_SUMMARY) $$output/main


.PHONY: $(TEST_TARGETS)
