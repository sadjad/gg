/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <string>
#include <cstring>
#include <getopt.h>

#include "thunk.hh"
#include "ggpaths.hh"
#include "path.hh"
#include "model-gcc.hh"

#include "toolchain.hh"

using namespace std;
using namespace gg::thunk;

template <typename E>
constexpr auto to_underlying( E e ) noexcept
{
    return static_cast<std::underlying_type_t<E>>( e );
}

enum class LDOption
{
  no_undefined = 1000, nostdlib
};

Thunk generate_thunk( int argc, char * argv[] )
{
  if ( argc < 2 ) {
    throw runtime_error( "not enough arguments" );
  }

  vector<string> original_args = gg::models::args_to_vector( argc, argv );

  struct option long_options[] = {
    { "entry",        required_argument, nullptr, 'e' },
    { "no-undefined", no_argument,       nullptr, to_underlying( LDOption::no_undefined ) },
    { "nostdlib",     no_argument,       nullptr, to_underlying( LDOption::nostdlib ) },
    { "output",       required_argument, nullptr, 'o' },

    { 0, 0, 0, 0 },
  };

  optind = 1; /* reset getopt */
  opterr = 0; /* turn off error messages */

  bool no_stdlib = false;
  vector<string> dep_args;

  string outfile;
  vector<InFile> infiles;
  vector<size_t> input_indexes;

  while ( true ) {
    const int opt = getopt_long( argc, argv, "-l:o:e:m:z:", long_options, NULL );

    if ( opt == -1 ) {
      break;
    }

    if ( opt == 1 ) {
      infiles.emplace_back( optarg );
      input_indexes.emplace_back( optind - 1 );
    }

    bool processed = true;

    switch( opt ) {
    case 'l':
    case 'e':
    case 'm':
    case 'z':
      break;

    case 'o':
      outfile = optarg;
      break;

    default:
      processed = false;
    }

    if ( not processed ) {
      switch ( static_cast<LDOption>( opt ) ) {
      case LDOption::nostdlib:
        no_stdlib = true;
        break;

      case LDOption::no_undefined:
        break;

      default:
        throw runtime_error( "unknown option: " + string( argv[ optind - 1 ] ) );
      }
    }
  }

  vector<string> all_args;

  if ( not no_stdlib ) {
    all_args.push_back( "-nostdlib" );

    for ( const string & dir : ld_search_path ) {
      all_args.push_back( "-rpath-link" );
      all_args.push_back( dir );
      infiles.emplace_back( dir, "", InFile::Type::DUMMY_DIRECTORY );
    }
  }

  return { outfile,
    { LD, all_args, {}, program_data( LD ).first },
    infiles
  };
}

int main( int argc, char * argv[] )
{
  gg::paths::fix_path_envar();

  Thunk thunk = generate_thunk( argc, argv );
  thunk.store();

  return 0;
}