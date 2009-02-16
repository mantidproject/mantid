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
  
};

#endif /*FILEVALIDATORTEST_H_*/
