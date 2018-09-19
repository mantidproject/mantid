#ifndef MANTID_MDEVENTS_ChangeQConventionTEST_H_
#define MANTID_MDEVENTS_ChangeQConventionTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidMDAlgorithms/BinMD.h"
#include "MantidMDAlgorithms/ChangeQConvention.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class ChangeQConventionTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    ChangeQConvention alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    Mantid::Kernel::ConfigService::Instance().setString("Q.convention",
                                                        "Inelastic");
    std::string wsName = "ChangeQConventionTest_ws";
    // Make a 3D MDEventWorkspace
    MDEventWorkspace3Lean::sptr ws =
        MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    // Make sure it is split
    ws->splitBox();

    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    ws->refreshCache();

    // There are this many boxes, so this is the max ID.
    TS_ASSERT_EQUALS(ws->getBoxController()->getMaxId(), 1001);

    ChangeQConvention alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsName));
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    auto ws2 = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName);
    TS_ASSERT_EQUALS("Crystallography", ws2->getConvention());
  }
};

#endif /* MANTID_MDEVENTS_ChangeQConventionTEST_H_ */
