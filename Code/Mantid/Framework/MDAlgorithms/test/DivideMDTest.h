#ifndef MANTID_MDALGORITHMS_DIVIDEMDTEST_H_
#define MANTID_MDALGORITHMS_DIVIDEMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/DivideMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::MDEvents::MDHistoWorkspace_sptr;

/** Note: More detailed tests for the underlying
 * operations are in BinaryOperationMDTest and
 * MDHistoWorkspaceTest.
 *
 */
class DivideMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    DivideMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("DivideMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 2./3., 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("DivideMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 2./3., 1e-5);
    BinaryOperationMDTestHelper::doTest("DivideMD", "scalar", "histo_A", "out", false /*fails*/);
  }

  void test_event_fails()
  {
    BinaryOperationMDTestHelper::doTest("DivideMD", "event_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("DivideMD", "scalar", "event_A", "out", false /*fails*/);
  }



};


#endif /* MANTID_MDALGORITHMS_DIVIDEMDTEST_H_ */
