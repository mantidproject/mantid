#ifndef MANTID_KERNEL_STRINGCONTAINSVALIDATORTEST_H_
#define MANTID_KERNEL_STRINGCONTAINSVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/StringContainsValidator.h"

using Mantid::Kernel::StringContainsValidator;
using namespace Mantid::API;

class StringContainsValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StringContainsValidatorTest *createSuite() { return new StringContainsValidatorTest(); }
  static void destroySuite( StringContainsValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_KERNEL_STRINGCONTAINSVALIDATORTEST_H_ */