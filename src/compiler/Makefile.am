AM_CPPFLAGS = -I$(srcdir)/../thunk $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS) $(NODEBUG_CXXFLAGS)

lib_LTLIBRARIES = libfsintercept.la
libfsintercept_la_SOURCES = fsintercept.cc
libfsintercept_la_LDFLAGS = -version-info 0:0:0 -fPIC -Wl,--whole-archive ../thunk/libthunk.a -Wl,--no-whole-archive
	 
