#ifndef MANTID_KERNEL_EQUALBINSCHECKERTEST_H_
#define MANTID_KERNEL_EQUALBINSCHECKERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/EqualBinsChecker.h"

using Mantid::Kernel::EqualBinsChecker;

class EqualBinsCheckerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EqualBinsCheckerTest *createSuite() { return new EqualBinsCheckerTest(); }
  static void destroySuite( EqualBinsCheckerTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_KERNEL_EQUALBINSCHECKERTEST_H_ */