#ifndef MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKLTEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertCWSDMDtoHKL.h"

using Mantid::MDAlgorithms::ConvertCWSDMDtoHKL;

class ConvertCWSDMDtoHKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWSDMDtoHKLTest *createSuite() { return new ConvertCWSDMDtoHKLTest(); }
  static void destroySuite( ConvertCWSDMDtoHKLTest *suite ) { delete suite; }

  void test_init() {
    ConvertCWSDMDtoHKL alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }


};

#endif /* MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKLTEST_H_ */
