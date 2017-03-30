#ifndef MANTID_KERNEL_DIFFRACTIONTEST_H_
#define MANTID_KERNEL_DIFFRACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Diffraction.h"

using Mantid::Kernel::Diffraction;

class DiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiffractionTest *createSuite() { return new DiffractionTest(); }
  static void destroySuite(DiffractionTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_KERNEL_DIFFRACTIONTEST_H_ */