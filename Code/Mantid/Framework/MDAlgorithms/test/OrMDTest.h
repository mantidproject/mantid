#ifndef MANTID_MDALGORITHMS_ORMDTEST_H_
#define MANTID_MDALGORITHMS_ORMDTEST_H_

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/OrMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class OrMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    OrMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("OrMD", "histo_A", "histo_zero", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("OrMD", "histo_zero", "histo_zero", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
  }

  void test_scalar_or_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("OrMD", "histo_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("OrMD", "event_A", "event_B", "out", false /*fails*/);
  }


};


#endif /* MANTID_MDALGORITHMS_ORMDTEST_H_ */
