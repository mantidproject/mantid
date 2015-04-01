#ifndef MANTID_MDALGORITHMS_EXPONENTIALMDTEST_H_
#define MANTID_MDALGORITHMS_EXPONENTIALMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/ExponentialMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class ExponentialMDTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    ExponentialMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo()
  {
    MDHistoWorkspace_sptr out;
    out = UnaryOperationMDTestHelper::doTest("ExponentialMD", "histo", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), exp(2.0), 1e-5);
  }

  void test_event_fails()
  {
    UnaryOperationMDTestHelper::doTest("ExponentialMD", "event", "out", false /* fails*/);
  }


};


#endif /* MANTID_MDALGORITHMS_EXPONENTIALMDTEST_H_ */
