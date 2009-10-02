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
      "The file must have one of these extensions: RAW, raw" )
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
  const std::string outputString("The file must have one of these extensions: c[a-z][a-z], h*, h??");
  TS_ASSERT_EQUALS( validator.isValid("fli.cp"), outputString )
  TS_ASSERT_EQUALS( validator.isValid("fli.c01"), outputString )
  TS_ASSERT_EQUALS( validator.isValid("fli.cxx"), "" )
  TS_ASSERT_EQUALS( validator.isValid("fli.hxx"), "" )
  TS_ASSERT_EQUALS( validator.isValid("fli.habc"), "" )
  TS_ASSERT_EQUALS( validator.isValid("fli.z"), outputString )
  TS_ASSERT_EQUALS( validator.isValid("fli.bpp"), outputString )
}

};

#endif /*FILEVALIDATORTEST_H_*/
