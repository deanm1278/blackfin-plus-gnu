AM_CPPFLAGS = -I$(top_srcdir)/hidapi/

# Other OS's
noinst_PROGRAMS = hidtest

hidtest_SOURCES = hidtest.cpp
hidtest_LDADD = $(top_builddir)/$(backend)/libhidapi.la
