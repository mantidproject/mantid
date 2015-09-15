#ifndef MANTID_MDALGORITHMS_ANDMDTEST_H_
#define MANTID_MDALGORITHMS_ANDMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/AndMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class AndMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    AndMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("AndMD", "histo_A", "histo_zero", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("AndMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
  }

  void test_scalar_or_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("AndMD", "histo_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("AndMD", "event_A", "event_B", "out", false /*fails*/);
  }


};


#endif /* MANTID_MDALGORITHMS_ANDMDTEST_H_ */
