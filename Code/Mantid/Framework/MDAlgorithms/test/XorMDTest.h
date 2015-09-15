#ifndef MANTID_MDALGORITHMS_XORMDTEST_H_
#define MANTID_MDALGORITHMS_XORMDTEST_H_

#include "MantidMDAlgorithms/XorMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidDataObjects/MDHistoWorkspace.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::MDAlgorithms;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class XorMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    XorMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("XorMD", "histo_A", "histo_zero", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 1.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("XorMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 0.0, 1e-5);
  }

  void test_scalar_or_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("XorMD", "histo_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("XorMD", "event_A", "event_B", "out", false /*fails*/);
  }


};


#endif /* MANTID_MDALGORITHMS_XORMDTEST_H_ */
