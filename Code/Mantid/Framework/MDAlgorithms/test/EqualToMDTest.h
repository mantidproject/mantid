#ifndef MANTID_MDALGORITHMS_EQUALTOMDTEST_H_
#define MANTID_MDALGORITHMS_EQUALTOMDTEST_H_

#include "MantidMDAlgorithms/EqualToMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class EqualToMDTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    EqualToMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("EqualToMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("EqualToMD", "histo_B", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("EqualToMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("EqualToMD", "scalar", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
  }

  void test_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("EqualToMD", "event_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("EqualToMD", "event_A", "event_B", "out", false /*fails*/);
  }

  void test_Tolerance()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("EqualToMD", "histo_A", "histo_B", "out", true, "Tolerance", "1.5");
    // Large enough tolerance to say that 2 == 3 (give or take 1.5)
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
  }



};


#endif /* MANTID_MDALGORITHMS_EQUALTOMDTEST_H_ */
