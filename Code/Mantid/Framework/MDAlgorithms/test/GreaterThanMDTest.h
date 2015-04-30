#ifndef MANTID_MDALGORITHMS_GREATERTHANMDTEST_H_
#define MANTID_MDALGORITHMS_GREATERTHANMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/GreaterThanMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class GreaterThanMDTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    GreaterThanMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("GreaterThanMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("GreaterThanMD", "histo_B", "histo_A", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("GreaterThanMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
  }

  void test_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("GreaterThanMD", "event_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("GreaterThanMD", "event_A", "event_B", "out", false /*fails*/);
  }

  void test_scalar_histo_fails()
  {
    BinaryOperationMDTestHelper::doTest("GreaterThanMD", "scalar", "histo_A", "out", false /*fails*/);
  }



};


#endif /* MANTID_MDALGORITHMS_GREATERTHANMDTEST_H_ */
