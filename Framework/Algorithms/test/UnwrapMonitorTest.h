#ifndef MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_
#define MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <string>
#include <set>

#include "MantidAlgorithms/UnwrapMonitor.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class UnwrapMonitorTest : public CxxTest::TestSuite {
private:
  void makeFakeWorkspace(std::string wsName) {
    Workspace2D_sptr test_in =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2, 10, 50);
    test_in->getAxis(0)->setUnit("TOF");

    // Add it to the workspace
    AnalysisDataService::Instance().add(wsName, test_in);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnwrapMonitorTest *createSuite() { return new UnwrapMonitorTest(); }
  static void destroySuite(UnwrapMonitorTest *suite) { delete suite; }

  void testLrefLessThanLd() {
    // setup
    std::string name("UnwrapMonitor");
    this->makeFakeWorkspace(name);
    Workspace2D_sptr ws =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(name);
    size_t nHist = ws->getNumberHistograms();

    // run the algorithm
    UnwrapMonitor algo;
    if (!algo.isInitialized())
      algo.initialize();
    algo.setPropertyValue("InputWorkspace", name);
    algo.setPropertyValue("OutputWorkspace", name);
    algo.setProperty("LRef", 11.0);
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    ws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(name);
    TS_ASSERT_EQUALS(nHist,
                     ws->getNumberHistograms()); // shouldn't drop histograms

    // Check some x values
    const Mantid::MantidVec outX = ws->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 23);
    TS_ASSERT_EQUALS(outX.front(), 0.0);
    TS_ASSERT_DELTA(outX.back(), 0.017982, 0.000001);

    // Check some y values
    const Mantid::MantidVec outY = ws->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 22);
    TS_ASSERT_EQUALS(outY[3], 0.0);
    TS_ASSERT_DELTA(outY[4], 2.991736, 0.000001);
    TS_ASSERT_DELTA(outY[8], 6.198347, 0.000001);
    TS_ASSERT_DELTA(outY[11], 3.818182, 0.000001);
    TS_ASSERT_EQUALS(outY[12], 0.0);

    double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.003692, 0.000001);

    AnalysisDataService::Instance().remove(name);
  }

  void testLrefGreaterThanLd() {
    // setup
    std::string name("UnwrapMonitor");
    this->makeFakeWorkspace(name);
    Workspace2D_sptr ws =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(name);
    size_t nHist = ws->getNumberHistograms();

    // run the algorithm
    UnwrapMonitor algo;
    if (!algo.isInitialized())
      algo.initialize();
    algo.setPropertyValue("InputWorkspace", name);
    algo.setPropertyValue("OutputWorkspace", name);
    algo.setProperty("LRef", 17.0);
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    ws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(name);
    TS_ASSERT_EQUALS(nHist,
                     ws->getNumberHistograms()); // shouldn't drop histograms

    double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.001582, 0.000001);

    AnalysisDataService::Instance().remove(name);
  }

  void testLrefEqualsLd() {
    // setup
    std::string name("UnwrapMonitor");
    this->makeFakeWorkspace(name);
    Workspace2D_sptr ws =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(name);
    size_t nHist = ws->getNumberHistograms();

    // run the algorithm
    UnwrapMonitor algo;
    if (!algo.isInitialized())
      algo.initialize();
    algo.setPropertyValue("InputWorkspace", name);
    algo.setPropertyValue("OutputWorkspace", name);
    algo.setProperty("LRef", 15.0);
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    ws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(name);
    TS_ASSERT_EQUALS(nHist,
                     ws->getNumberHistograms()); // shouldn't drop histograms

    double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.000264, 0.000001);

    AnalysisDataService::Instance().remove(name);
  }
};

#endif /* MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_ */
