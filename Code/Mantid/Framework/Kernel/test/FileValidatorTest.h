#ifndef FILEVALIDATORTEST_H_
#define FILEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FileValidator.h"
#include <Poco/File.h>

using namespace Mantid::Kernel;

class FileValidatorTest: public CxxTest::TestSuite
{
public:

void testVectorConstructor()
{
  std::vector<std::string> vec;
  vec.push_back("raw");
  vec.push_back("RAW");
  FileValidator v(vec);
  TS_ASSERT_EQUALS  ( v.allowedValues().size(), 2 );
}

void testPassesOnExistentFile()
{
  //Create two files, one with the extension within the validator and one without

  const std::string file_stub = "scratch.";
  const std::string ext1 = "txt";
  const std::string ext2 = "raw";
  Poco::File txt_file(file_stub + ext1);
  Poco::File raw_file(file_stub + ext2);
    
  try
  {
    txt_file.createFile();
    raw_file.createFile();
  }
  catch(std::exception & )
  {
    TS_FAIL("Error creating test file for \"testPassesOnExistentFile\" test.");
  }
  
  //FileValidator will suggest txt files as correct extension
  std::vector<std::string> vec(1, "txt");
  FileValidator v1(vec);

  
  TS_ASSERT_EQUALS( v1.isValid(txt_file.path()), "" );
  // Not correct extension but the file exists so we allow it
  TS_ASSERT_EQUALS( v1.isValid(raw_file.path()), "" );

  txt_file.remove();
  raw_file.remove();
}

void testPassesForMoreComplicatedExtensions()
{
  //More general test cases (Refs #1302)
  const std::string file_stub = "scratch";
  const std::string ext1 = ".tar.gz";
  const std::string ext2 = "_event.dat";
  Poco::File txt_file(file_stub + ext1);
  Poco::File raw_file(file_stub + ext2);
  try
  {
    txt_file.createFile();
    raw_file.createFile();
  }
  catch(std::exception & )
  {
    TS_FAIL("Error creating test file for \"testPassesForMoreComplicatedExtensions\" test.");
  }

  //FileValidator will suggest txt files as correct extension
  std::vector<std::string> vec(1, ".tar.gz");
  FileValidator v1(vec);

  TS_ASSERT_EQUALS( v1.isValid(txt_file.path()), "" );
  // Not correct extension but the file exists so we allow it
  TS_ASSERT_EQUALS( v1.isValid(raw_file.path()), "" );

  txt_file.remove();
  raw_file.remove();
}

void testFailsOnNonexistentFile()
{
  std::string NoFile("myJunkFile_hgfvj.cpp");
  std::vector<std::string> vec;
  vec.push_back("cpp");
  FileValidator v(vec);
  TS_ASSERT_EQUALS( v.isValid(NoFile),
      "File \"" + NoFile + "\" not found" );
}

void testPassesOnNonexistentFile()
{
  std::string NoFile("myJunkFile_hgfvj.cpp");
  std::vector<std::string> vec;
  vec.push_back("cpp");
  FileValidator v(vec, false);
  TS_ASSERT_EQUALS( v.isValid(NoFile), "" );
}

void testFailsOnEmptyFileString()
{
  FileValidator file_val;
  TS_ASSERT_EQUALS( file_val.isValid(""), "File \"\" not found" );
}

};

#endif /*FILEVALIDATORTEST_H_*/
