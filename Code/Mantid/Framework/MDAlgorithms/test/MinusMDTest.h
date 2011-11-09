#ifndef MANTID_MDALGORITHMS_MINUSMDTEST_H_
#define MANTID_MDALGORITHMS_MINUSMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/MinusMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::MDEvents::MDHistoWorkspace_sptr;

class MinusMDTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    MinusMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MinusMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), -1.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MinusMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), -1.0, 1e-5);
    BinaryOperationMDTestHelper::doTest("MinusMD", "scalar", "histo_A", "out", false /*fails*/);
  }

  void test_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("MinusMD", "event_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("MinusMD", "scalar", "event_A", "out", false /*fails*/);
  }



};


#endif /* MANTID_MDALGORITHMS_MINUSMDTEST_H_ */
