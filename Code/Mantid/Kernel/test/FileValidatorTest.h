#ifndef FILEVALIDATORTEST_H_
#define FILEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FileValidator.h"

using namespace Mantid::Kernel;

class FileValidatorTest : public CxxTest::TestSuite
{
public:

  void testVectorConstructor()
  {
    std::vector<std::string> vec;
    vec.push_back("raw");
    vec.push_back("RAW");
    FileValidator v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 2 )
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
    std::vector<std::string> vec;
    vec.push_back("raw");
    vec.push_back("RAW");
    FileValidator v(vec);
    TS_ASSERT(!v.isValid("FileValidatorTest.cpp"));
  }

  void testPassesOnRightExtension()
  {
    std::vector<std::string> vec;
    vec.push_back("cpp");
    FileValidator v(vec);
    TS_ASSERT(v.isValid("FileValidatorTest.cpp"));
  }
  
  void testFailsOnNonexistentFile()
  {
    std::vector<std::string> vec;
    vec.push_back("cpp");
    FileValidator v(vec);
    TS_ASSERT(!v.isValid("junk.cpp"));
  }
  
   void testPassesOnExistentFile()
  {
    std::vector<std::string> vec;
    vec.push_back("cpp");
    FileValidator v(vec);
    TS_ASSERT(v.isValid("FileValidatorTest.cpp"));
  }

  void testFailsOnEmptyFileString()
  {
    FileValidator file_val;
    TS_ASSERT( !file_val.isValid("") );
  }

  void testPassesOnWildcardExtensions()
  {
    std::vector<std::string> exts;
    exts.push_back("c[a-z][a-z]");
    exts.push_back("h??");
    exts.push_back("h*");
    FileValidator validator(exts, false);
    TS_ASSERT( validator.isValid("FileValidatorTest.cpp") );
    TS_ASSERT( validator.isValid("FileValidatorTest.cxx") );
    TS_ASSERT( validator.isValid("FileValidatorTest.hxx") );
    TS_ASSERT( validator.isValid("FileValidatorTest.habc") );
  }

  void testFailsOnWrongWildcardExtensions()
  {
    std::vector<std::string> exts;
    exts.push_back("c[a-z][a-z]");
    FileValidator validator(exts, false);
    TS_ASSERT( !validator.isValid("FileValidatorTest.c01") );
    TS_ASSERT( !validator.isValid("FileValidatorTest.bpp") );
  }
  
};

#endif /*FILEVALIDATORTEST_H_*/
