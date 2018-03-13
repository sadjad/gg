/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <unistd.h>
#include <cstring>
#include <iostream>
#include <google/protobuf/text_format.h>

#include "thunk/thunk_reader.hh"
#include "thunk/thunk_writer.hh"
#include "thunk/thunk.hh"
#include "util/exception.hh"
#include "util/temp_file.hh"

using namespace std;
using namespace google::protobuf;
using namespace gg::thunk;

void usage( const char * argv0 )
{
  cerr << argv0 << endl;
}

int main( int argc, char * argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort();
    }

    if ( argc != 1 ) {
      usage( argv[ 0 ] );
      return EXIT_FAILURE;
    }

    Thunk original_thunk {
      {
        "FUNCTIONHASH", { "f", "arg1", "arg2", "arg3", "arg4", "arg5", "arg6" },
        { "envar1=A", "envar2=B", "envar3=C" },
      },
      {
        "XOBJ1", "XOBJ2", "VOBJ3", "VOBJ4", "TOBJ5", "TOBJ6"
      },
      {
        "output1", "output2", "output3", "output4"
      }
    };

    TempFile temp_file { "output" };
    const string contents = ThunkWriter::serialize_thunk( original_thunk );
    roost::atomic_create( contents, temp_file.name() );

    // Now reading it back
    ThunkReader thunk_reader { temp_file.name() };
    Thunk thunk = thunk_reader.read_thunk();

    if ( thunk == original_thunk ) {
      return EXIT_SUCCESS;
    }
    else {
      return EXIT_FAILURE;
    }

  }
  catch ( const exception &  e ) {
    print_exception( argv[ 0 ], e );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
