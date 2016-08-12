#ifndef GETEIMONDET2TEST_H_
#define GETEIMONDET2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/GetEiMonDet2.h"

using namespace Mantid::Algorithms;

class GetEiMonDet2Test : public CxxTest::TestSuite {
public:
  static GetEiMonDet2Test *createSuite() {
    return new GetEiMonDet2Test();
  }
  static void destroySuite(GetEiMonDet2Test *suite) { delete suite; }

  void testName() {
    GetEiMonDet2 algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "GetEiMonDet")
  }

  void testVersion() {
    GetEiMonDet2 algorithm;
    TS_ASSERT_EQUALS(algorithm.version(), 2)
  }

  void testInit() {
    GetEiMonDet2 algorithm;
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
  }
};

#endif // GETEIMONDET2TEST_H_
