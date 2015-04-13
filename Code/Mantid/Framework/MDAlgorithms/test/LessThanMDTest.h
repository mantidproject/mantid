#ifndef MANTID_MDALGORITHMS_LESSTHANMDTEST_H_
#define MANTID_MDALGORITHMS_LESSTHANMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/LessThanMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class LessThanMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    LessThanMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("LessThanMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("LessThanMD", "histo_B", "histo_A", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("LessThanMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
  }

  void test_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("LessThanMD", "event_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("LessThanMD", "event_A", "event_B", "out", false /*fails*/);
  }

  void test_scalar_histo_fails()
  {
    BinaryOperationMDTestHelper::doTest("LessThanMD", "scalar", "histo_A", "out", false /*fails*/);
  }


};


#endif /* MANTID_MDALGORITHMS_LESSTHANMDTEST_H_ */
