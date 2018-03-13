/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "factory.hh"

#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>

#include "ggutils.hh"
#include "manifest.hh"
#include "placeholder.hh"
#include "thunk_writer.hh"
#include "util/exception.hh"
#include "util/file_descriptor.hh"
#include "util/optional.hh"
#include "util/path.hh"
#include "util/tokenize.hh"

using namespace std;
using namespace gg;

ThunkFactory::Data::Data( const string & filename,
                          const string & real_filename,
                          const ObjectType & type,
                          const string & hash )
  : filename_( roost::path( filename ).lexically_normal().string() ),
    real_filename_( ( real_filename.length() ) ? real_filename : filename_ ),
    hash_( hash ), type_( type )
{
  if ( hash_.length() ) {
    return;
  }

  Optional<ThunkPlaceholder> placeholder = ThunkPlaceholder::read( real_filename_ );

  if ( placeholder.initialized() ) {
    type_ = ObjectType::Thunk;
    hash_ = placeholder->content_hash();
  }
  else {
    type_ = type;
    hash_ = compute_hash();
  }
}

string ThunkFactory::Data::compute_hash() const
{
  /* do we have this hash in cache? */
  struct stat file_stat;
  CheckSystemCall( "stat", stat( real_filename_.c_str(), &file_stat ) );

  const auto cache_entry_path = gg::paths::hash_cache_entry( real_filename_, file_stat );

  if ( roost::exists( cache_entry_path ) ) {
    FileDescriptor cache_file { CheckSystemCall( "open",
                                                 open( cache_entry_path.string().c_str(), O_RDONLY ) ) };
    string cache_entry;
    while ( not cache_file.eof() ) { cache_entry += cache_file.read(); }

    vector<string> cache_contents = split( cache_entry, " " );

    if ( cache_contents.size() != 6 ) {
      throw runtime_error( "bad cache entry: " + cache_entry_path.string() );
    }

    if ( cache_contents.at( 0 ) == to_string( file_stat.st_size )
         and cache_contents.at( 1 ) == to_string( file_stat.st_mtim.tv_sec )
         and cache_contents.at( 2 ) == to_string( file_stat.st_mtim.tv_nsec )
         and cache_contents.at( 3 ) == to_string( file_stat.st_ctim.tv_sec )
         and cache_contents.at( 4 ) == to_string( file_stat.st_ctim.tv_nsec ) ) {
      /* cache hit! */
      return cache_contents.at( 5 );
    }
  }

  /* not a cache hit, so need to compute hash ourselves */

  FileDescriptor file { CheckSystemCall( "open (" + real_filename_ + ")",
                                         open( real_filename_.c_str(), O_RDONLY ) ) };

  string contents;
  while ( not file.eof() ) { contents += file.read(); }

  const string computed_hash = gg::hash::compute( contents, type_ );

  /* make a cache entry */
  atomic_create( to_string( file_stat.st_size ) + " "
                 + to_string( file_stat.st_mtim.tv_sec ) + " "
                 + to_string( file_stat.st_mtim.tv_nsec ) + " "
                 + to_string( file_stat.st_ctim.tv_sec ) + " "
                 + to_string( file_stat.st_ctim.tv_nsec ) + " "
                 + computed_hash,
                 cache_entry_path );

  return computed_hash;
}

std::string ThunkFactory::generate( const Function & function,
                                    const std::vector<Data> & data,
                                    const std::vector<Data> & executables,
                                    const std::vector<Output> & outputs,
                                    const bool generate_manifest,
                                    const std::vector<std::string> & dummy_dirs,
                                    const bool create_placeholder,
                                    const bool collect_data )
{
  vector<string> thunk_data;
  vector<string> thunk_executables;
  vector<string> thunk_outputs;

  thunk::Function thunk_function { function };

  if ( generate_manifest ) {
    FileManifest manifest;

    for ( const Data & datum : data ) {
      thunk_data.push_back( datum.hash() );

      if ( datum.filename().length() > 0 ) {
        manifest.add_filename_to_hash( datum.filename(), datum.hash() );
      }
    }

    for ( const Data & datum : executables ) {
      thunk_executables.push_back( datum.hash() );

      if ( datum.filename().length() > 0 ) {
        manifest.add_filename_to_hash( datum.filename(), datum.hash() );
      }
    }

    for ( const string & dir : dummy_dirs ) {
      manifest.add_dummy_directory( dir );
    }

    for ( const Output & output : outputs ) {
      thunk_outputs.push_back( output.tag() );

      if ( output.filename().initialized() ) {
        manifest.add_output_tag( *output.filename(), output.tag() );
      }
    }

    string manifest_data = manifest.serialize();
    string manifest_hash = gg::hash::compute( manifest_data,
                                              ObjectType::Value );
    thunk_data.push_back( manifest_hash );
    roost::atomic_create( manifest_data,
                          gg::paths::blob_path( manifest_hash ) );
    thunk_function.envars().push_back( "GG_MANIFEST=" + thunk::data_placeholder( manifest_hash ) );
  }

  if ( collect_data ) {
    auto fn_collect =
      [] ( const Data & datum )
      {
        roost::path source_path = datum.real_filename();
        roost::path target_path = gg::paths::blob_path( datum.hash() );

        if ( not roost::exists( target_path ) ) {
          roost::copy_then_rename( source_path, target_path );
        }
      };

    for ( const Data & datum : data ) { fn_collect( datum ); }
    for ( const Data & datum : executables ) { fn_collect( datum ); }
  }

  string hash = ThunkWriter::write_thunk( { thunk_function,
                                            thunk_data,
                                            thunk_executables,
                                            thunk_outputs } );

  if ( create_placeholder and outputs.at( 0 ).filename().initialized() ) {
    ThunkPlaceholder placeholder { hash };
    placeholder.write( *outputs.at( 0 ).filename() );
  }

  return hash;
}
