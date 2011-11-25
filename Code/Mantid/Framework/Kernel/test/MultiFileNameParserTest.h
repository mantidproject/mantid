#ifndef MANTID_KERNEL_MULTIFILENAMEPARSERTEST_H_
#define MANTID_KERNEL_MULTIFILENAMEPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MultiFileNameParser.h"

using namespace Mantid::Kernel;

class MultiFileNameParserTest : public CxxTest::TestSuite
{
public:
  void basic(const std::string & multiFileName)
  {
    MultiFileNameParser parser;
    std::string error;

    TS_ASSERT_THROWS_NOTHING( error = parser.parse(multiFileName) );
    TS_ASSERT_EQUALS(error, std::string(""));

    // @TODO test contents
  }

  void test_single()
  {
    basic("c:/data/INST1.ext");
  }

  void test_list()
  {
    basic("c:/data/INST1,2,3,4.ext");
  }

  void test_range()
  {
    basic("c:/data/INST1:4.ext");
  }

  void test_steppedRange()
  {
    basic("c:/data/INST1:4:2.ext");
  }

  void test_addedList()
  {
    basic("c:/data/INST1+2.ext");
  }

  void test_addedRange()
  {
    basic("c:/data/INST1-4.ext");
  }

  void test_addedSteppedRange()
  {
    basic("c:/data/INST1-4:2.ext");
  }
};

#endif /* MANTID_KERNEL_MULTIFILENAMEPARSERTEST_H_ */