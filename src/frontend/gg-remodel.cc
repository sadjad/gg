/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>

#include "thunk/graph.hh"
#include "thunk/placeholder.hh"
#include "util/exception.hh"

using namespace std;

void usage( const char * argv0 )
{
  cerr << argv0 << " THUNK-PLACEHOLDERS..." << endl;
}

int main( int argc, char ** argv )
{
  try {
    if ( argc <= 0 ) {
      abort();
    }

    if ( argc < 2 ) {
      usage( argv[ 0 ] );
      return EXIT_FAILURE;
    }

    vector<string> target_filenames;
    vector<string> target_hashes;

    for ( int i = 1; i < argc; i++ ) {
      target_filenames.emplace_back( argv[ i ] );
    }

    for ( const string & target_filename : target_filenames ) {
      Optional<ThunkPlaceholder> placeholder = ThunkPlaceholder::read( target_filename );

      if ( not placeholder.initialized() ) {
        throw runtime_error( "not a placeholder: " + target_filename );
      }

      target_hashes.emplace_back( placeholder->content_hash() );
    }

    DependencyGraph graph { target_hashes };

    unordered_set<string> ready_to_process = graph.ready_to_execute();

    while ( not ready_to_process.empty() ) {
      const auto & hash_it = ready_to_process.begin();
      cerr << *hash_it << endl;

      unordered_set<string> next_thunks = graph.remove_thunk( *hash_it );
      ready_to_process.erase( hash_it );
      ready_to_process.insert( next_thunks.begin(), next_thunks.end() );
    }
  }
  catch ( const exception &  e ) {
    print_exception( argv[ 0 ], e );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
