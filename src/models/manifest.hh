/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef MANIFEST_HH
#define MANIFEST_HH

#include <vector>
#include <string>

class FileManifest
{
private:
  typedef std::pair<std::string, std::string> FileToHashEntry;
  typedef std::string DummyDirectoryEntry;
  typedef std::pair<std::string, std::string> OutputTagEntry;

  std::vector<FileToHashEntry> filename_to_hashes_ {};
  std::vector<DummyDirectoryEntry> dummy_directories_ {};
  std::vector<OutputTagEntry> output_tags_ {};

public:
  FileManifest() {}

  void add_filename_to_hash( const std::string & filename, const std::string & hash );
  void add_dummy_directory( const std::string & dummy_dir );
  void add_output_tag( const std::string & filename, const std::string & tag );

  std::string serialize() const;
};

#endif /* MANIFEST_HH */
