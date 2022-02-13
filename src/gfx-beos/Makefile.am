AM_CPPFLAGS  = @UAE_CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS    = @UAE_CFLAGS@
AM_CXXFLAGS  = $(CFLAGS) @UAE_CFLAGS@ -Wno-multichar

INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)


noinst_LIBRARIES = libgfxdep.a

libgfxdep_a_SOURCES = be-UAE.cpp be-Window.cpp be-Input.cpp

noinst_HEADERS = gfx.h be-UAE.h be-Window.h
