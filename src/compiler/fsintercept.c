#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

typedef int ( *orig_open_f_type )( const char *pathname, int flags );
 
int open( const char *pathname, int flags, ... )
{
  orig_open_f_type orig_open;
  orig_open = (orig_open_f_type)dlsym( RTLD_NEXT, "open" );
  printf( "DANITER INTERCEPTING OPEN %s\n", pathname );
  return orig_open( pathname, flags );
}
