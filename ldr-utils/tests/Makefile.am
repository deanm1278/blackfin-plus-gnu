EXTRA_DIST = fake-loadee.c Makefile.in atlocal.in package.m4.in elfs/POST.dxe elfs/bad.elf elfs/bss.elf elfs/final-split.elf elfs/section.elf ldrs/bf548.ldr ldrs/spi.ldr ldrs/uart.ldr $(AT_FILES) $(TESTSUITE)
AT_FILES = create.at dump.at load.at local.at show.at source.at testsuite.at usage.at
TESTSUITE = $(srcdir)/testsuite
DISTCLEANFILES = atconfig

INCLUDES = -I$(top_srcdir)

check_PROGRAMS = fake-loadee
fake_loadee_LDADD = @PTY_LIB@

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

installcheck-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || \
		$(SHELL) '$(TESTSUITE)' --clean

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(AT_FILES)
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@
