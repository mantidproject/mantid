#ifndef MANTID_MDALGORITHMS_MULTIPLYMDTEST_H_
#define MANTID_MDALGORITHMS_MULTIPLYMDTEST_H_

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/MultiplyMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

using Mantid::coord_t;
using Mantid::signal_t;

/** Note: More detailed tests for the underlying
 * operations are in BinaryOperationMDTest and
 * MDHistoWorkspaceTest.
 *
 */
class MultiplyMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    MultiplyMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MultiplyMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 6.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MultiplyMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 6.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("MultiplyMD", "scalar", "histo_A", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 6.0, 1e-5);
  }

  void test_event_event_or_histo_fails()
  {
    BinaryOperationMDTestHelper::doTest("MultiplyMD", "event_A", "histo_A", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("MultiplyMD", "histo_A", "event_A", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("MultiplyMD", "event_A", "event_A", "out", false /*fails*/);
  }


  /** Get a MDEventWorkspace and check that all events have the given signal/error */
  void checkMDEWSignal(std::string wsName, signal_t expectedSignal, signal_t expectedError)
  {
    IMDEventWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    TS_ASSERT(ws); if (!ws) return;
    IMDIterator* it = ws->createIterator(NULL);
    do
    {
      TS_ASSERT_EQUALS( it->getNumEvents(), 1);
      TS_ASSERT_DELTA( it->getInnerSignal(0), expectedSignal, 1e-5);
      TS_ASSERT_DELTA( it->getInnerError(0), expectedError, 1e-5);
    }
    while (it->next());
  }

  /// Multiply events by a scalar
  void test_event_scalar()
  {
    BinaryOperationMDTestHelper::doTest("MultiplyMD", "event_A", "scalar", "out");
    checkMDEWSignal("out", 3.0, sqrt(12.0));
    BinaryOperationMDTestHelper::doTest("MultiplyMD", "scalar", "event_A", "out");
    checkMDEWSignal("out", 3.0, sqrt(12.0));
  }

};


#endif /* MANTID_MDALGORITHMS_MULTIPLYMDTEST_H_ */
