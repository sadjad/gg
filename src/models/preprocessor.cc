/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "gcc.hh"

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <sys/fcntl.h>
#include <unistd.h>

#include "thunk/ggutils.hh"
#include "thunk/thunk_reader.hh"
#include "thunk/thunk.hh"
#include "util/exception.hh"
#include "util/system_runner.hh"
#include "util/temp_file.hh"

using namespace std;
using namespace boost;

vector<string> GCCModelGenerator::parse_dependencies_file( const string & dep_filename,
                                                           const string & target_name )
{
  vector<string> dependencies;

  string target_str = target_name + ": ";

  ifstream depin { dep_filename };
  string line;
  bool found_target = false;

  while ( getline( depin, line ) ) {
    if ( line.length() < 2 ) {
      if ( found_target ) { break; }
      else { continue; }
    }

    if ( not found_target and
         line.compare( 0, target_str.size(), target_str ) == 0 ) {
      line = line.substr( line.find( ':' ) + 1, line.length() );
      found_target = true;
    }
    else if ( found_target and
              line.find( ":" ) != string::npos ) {
      break;
    }

    if ( not found_target ) {
      continue;
    }

    line = line.substr( 1, line.length() );

    if ( line[ line.length() - 1 ] == '\\' ) {
      line = line.substr( 0, line.length() - 2 );
    }

    if ( line == "\\" ) {
      continue;
    }

    tokenizer<escaped_list_separator<char>> tok( line, { "\\", " ", "\"\'" } );

    for ( auto t = tok.begin(); t != tok.end(); t++ ) {
      dependencies.push_back( *t );
    }
  }

  return dependencies;
}

vector<string> GCCModelGenerator::generate_dependencies_file( const vector<string> & option_args,
                                                              const string & output_name,
                                                              const string & target_name )
{
  vector<string> args;
  args.reserve( 1 + option_args.size() );

  if ( operation_mode_ == OperationMode::GCC ) {
    args.push_back( "gcc-7" );
  }
  else {
    args.push_back( "g++-7" );
  }

  args.insert( args.end(), option_args.begin(), option_args.end() );

  string output_filename;

  const bool has_dependencies_option = find_if(
    args.begin(), args.end(),
    []( const string & opt )
    {
      return ( opt == "-M" ) or ( opt == "-MF" ) or ( opt == "-MM" ) or
             ( opt == "-MG" ) or ( opt == "-MP" ) or ( opt == "-MQ" ) or
             ( opt == "-MD" ) or ( opt == "-MMD" );
    } ) != end( args );

  if ( has_dependencies_option ) {
    auto m_search = find( args.begin(), args.end(), "-M" );
    auto mf_search = find( args.begin(), args.end(), "-MF" );
    auto md_search = find( args.begin(), args.end(), "-MD" );

    if ( mf_search == end( args ) ) {
      throw runtime_error( "cannot produce dependencies file without -MF option" );
    }

    if ( md_search != end( args ) ) {
      args.erase( md_search );

      if ( m_search != end( args ) ) {
        args.push_back( "-M" );
      }
    }
  }
  else {
    args.push_back( "-M" );
    args.push_back( "-MF" );
    args.push_back( output_name );
    args.push_back( "-MT" );
    args.push_back( target_name );
  }

  run( args[ 0 ], args, {}, true, true );

  return parse_dependencies_file( output_name, target_name );
}
