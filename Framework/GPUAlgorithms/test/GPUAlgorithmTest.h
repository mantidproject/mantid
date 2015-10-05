#ifndef MANTID_GPUALGORITHMS_GPUALGORITHMTEST_H_
#define MANTID_GPUALGORITHMS_GPUALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGPUAlgorithms/GPUAlgorithm.h"

using namespace Mantid;
using namespace Mantid::GPUAlgorithms;
using namespace Mantid::API;

class GPUAlgorithmTest : public CxxTest::TestSuite {
public:
  void test_1() {}
};

#endif /* MANTID_GPUALGORITHMS_GPUALGORITHMTEST_H_ */
