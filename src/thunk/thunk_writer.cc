/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "thunk_writer.hh"

#include <iostream>
#include <cajun/json/writer.h>
#include <fstream>
// TODO : should local header files be first or system header files?

using namespace std;

bool ThunkWriter::write_thunk( Thunk thunk )
{
  // TODO : Remove print statements
  std::stringstream stream;
  json::Writer::Write( thunk.to_json(), stream );
  cout << stream.str() << endl;

  ofstream outfile;
  outfile.open( thunk.get_outfile() );
  outfile << stream.rdbuf();
  outfile.close();

  return true;
}

Thunk ThunkWriter::read_thunk( string filename ) {
  fstream file( filename );
  json::Object jobj;
  json::Reader::Read(jobj, file);

  const json::String& jstr = jobj["outfile"];
  string outfile = jstr.Value();
  vector<string> cmd;
  json::Array& array = jobj["args"];
  json::Array::const_iterator it(array.Begin()), itend(array.End());
  for (; it != itend; ++it){
    json::String& s = *it;
    string st = s.Value();
    cmd.push_back( st );
  }
  ThunkFunc thunkfunc(cmd);
  InFileDescriptor infile( jobj[ "infiles" ][ 0 ][ "filename" ].Value() );
  vector<InFileDescriptor> infiles = {infile};
  return Thunk(outfile, thunkfunc, infiles);
}