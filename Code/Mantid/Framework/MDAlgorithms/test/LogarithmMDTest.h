#ifndef MANTID_MDALGORITHMS_LOGARITHMMDTEST_H_
#define MANTID_MDALGORITHMS_LOGARITHMMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/LogarithmMD.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class LogarithmMDTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    LogarithmMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo()
  {
    MDHistoWorkspace_sptr out;
    out = UnaryOperationMDTestHelper::doTest("LogarithmMD", "histo", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), std::log(2.0), 1e-5);
  }

  void test_histo_with_not_Natural()
  {
    MDHistoWorkspace_sptr out;
    out = UnaryOperationMDTestHelper::doTest("LogarithmMD", "histo", "out", true, "Natural", "0");
    TS_ASSERT_DELTA( out->getSignalAt(0), std::log10(2.0), 1e-5);
  }

  void test_event_fails()
  {
    UnaryOperationMDTestHelper::doTest("LogarithmMD", "event", "out", false /* fails*/);
  }



};


#endif /* MANTID_MDALGORITHMS_LOGARITHMMDTEST_H_ */
