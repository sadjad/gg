#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include "thunk.hh"
#include "thunk_writer.hh"

typedef int ( *orig_open_f_type )( const char *pathname, int flags );

using namespace std;

const size_t PATH_MAX = 512;

string get_thunk_filename() {
  char cmdline[PATH_MAX];
  ssize_t len = readlink("/proc/self/cmdline", cmdline, sizeof(cmdline));
  if (len == -1 || len == sizeof(cmdline))
    len = 0;
  cmdline[len] = '\0';
  cout << cmdline << endl;
  return string(cmdline);
}

Thunk get_current_thunk() {
  string curr_thunk_name = get_thunk_filename();

  return ThunkWriter::read_thunk( curr_thunk_name );
}

extern "C" int open( const char *pathname, int flags, ... )
{
  orig_open_f_type orig_open;
  orig_open = (orig_open_f_type)dlsym( RTLD_NEXT, "open" );
  printf( "DANITER INTERCEPTING OPEN %s\n", pathname );

  // Read current thunk
  Thunk thunk = get_current_thunk();

  // Get hash of pathname
  // TODO : Should we compute the hash given the file or just look up the hash in the thunk?

  // Repace pathname

  return orig_open( pathname, flags );
}
