#ifndef FILEVALIDATORTEST_H_
#define FILEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FileValidator.h"

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
  TS_ASSERT_EQUALS  ( v.allowedValues().size(), 2 )
}

void testGetType()
{
  std::vector<std::string> vec;
  vec.push_back("raw");
  vec.push_back("RAW");
  FileValidator v(vec);
  TS_ASSERT_EQUALS( v.getType(), "file" )
}

void testFileMustExist()
{
  std::vector<std::string> vec;
  vec.push_back("cpp");
  FileValidator v(vec);
  TS_ASSERT( v.fileMustExist() )
}

void testFileDoesNotNeedToExist()
{
  std::vector<std::string> vec;
  vec.push_back("cpp");
  FileValidator v(vec, false);
  TS_ASSERT( !v.fileMustExist() )
}

void testFailsOnWrongExtension()
{
  //You can't find this file?  It just has to be a file that is in the test directory of all build servers, with the same, case.  Any ideas?  If you change the extension then also change the file extension test below ... Steve Williams
  std::string testFile("RunTests.bat");

  std::vector<std::string> vec;
  vec.push_back("raw");

  FileValidator v1(vec);
  TS_ASSERT_EQUALS( v1.isValid(testFile),
      "The file must have extension raw" )

  vec.push_back("RAW");
  FileValidator v2(vec);
  TS_ASSERT_EQUALS( v2.isValid(testFile),
      "The file must have one of these extensions: raw, RAW" )

  //the validator will not print lots of lots of extensions, check the limit
  for (unsigned short i = 2; i <= 20; i++)
  {
    char a[3];
    a[0] = '.';
    a[1] = 'n';
    a[2] = char(i);
    vec.push_back( std::string(a, 3) );
  }
  FileValidator v3(vec);
  TS_ASSERT_EQUALS( v3.isValid(testFile),
      "The file \"" + testFile + "\" has the wrong extension")
}

void testPassesOnRightExtension()
{
  std::string testFile("runTests.bat");

  std::vector<std::string> vec;
  vec.push_back("bat");
  FileValidator v(vec);
  TS_ASSERT_EQUALS( v.isValid(testFile), "" )
}

void testFailsOnNonexistentFile()
{
  std::string NoFile("myJunkFile_hgfvj.cpp");
  std::vector<std::string> vec;
  vec.push_back("cpp");
  FileValidator v(vec);
  TS_ASSERT_EQUALS( v.isValid(NoFile),
      "File \"" + NoFile + "\" not found" )
}

void testFailsOnEmptyFileString()
{
  FileValidator file_val;
  TS_ASSERT_EQUALS( file_val.isValid(""),
      "File \"\" not found" )
}

void testPassesOnWildcardExtensions()
{
  std::vector<std::string> exts;
  exts.push_back("c[a-z][a-z]");
  exts.push_back("h??");
  exts.push_back("h*");
  FileValidator validator(exts, false);
  TS_ASSERT_EQUALS( validator.isValid("fli.cpp"), "" )
  TS_ASSERT_EQUALS( validator.isValid("fli.cp"),
      "The file must have one of these extensions: c[a-z][a-z], h??, h*" )
  TS_ASSERT_EQUALS( validator.isValid("fli.c01"),
      "The file must have one of these extensions: c[a-z][a-z], h??, h*" )
  TS_ASSERT_EQUALS( validator.isValid("fli.cxx"), "" )
  TS_ASSERT_EQUALS( validator.isValid("fli.hxx"), "" )
  TS_ASSERT_EQUALS( validator.isValid("fli.habc"), "" )
  TS_ASSERT_EQUALS( validator.isValid("fli.z"),
      "The file must have one of these extensions: c[a-z][a-z], h??, h*" )
  TS_ASSERT_EQUALS( validator.isValid("fli.bpp"),
      "The file must have one of these extensions: c[a-z][a-z], h??, h*" )
}

};

#endif /*FILEVALIDATORTEST_H_*/
