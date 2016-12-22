#ifndef MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_
#define MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <string>

#include "MantidAlgorithms/UnwrapMonitor.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class UnwrapMonitorTest : public CxxTest::TestSuite {
private:
  const MatrixWorkspace_sptr makeFakeWorkspace() {
    Workspace2D_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2, 3, 50);
    testWS->getAxis(0)->setUnit("TOF");
    return testWS;
  }

  void setupAlgorithm(UnwrapMonitor &algo, const MatrixWorkspace_sptr inWS,
                      const double lref) {
    if (!algo.isInitialized())
      algo.initialize();
    algo.setChild(true);
    algo.setProperty("InputWorkspace", inWS);
    algo.setPropertyValue("OutputWorkspace", "outWS");
    algo.setProperty("LRef", lref);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnwrapMonitorTest *createSuite() { return new UnwrapMonitorTest(); }
  static void destroySuite(UnwrapMonitorTest *suite) { delete suite; }

  void testLrefLessThanLd() {
    // setup
    MatrixWorkspace_sptr inWS = this->makeFakeWorkspace();
    UnwrapMonitor algo;
    setupAlgorithm(algo, inWS, 11.0);

    // run the algorithm
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS =
        algo.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    // Check some x values
    const Mantid::MantidVec outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 23);
    TS_ASSERT_DELTA(outX.front(), 0.0, 1e-6);
    TS_ASSERT_DELTA(outX.back(), 0.017982, 1e-6);

    // Check some y values
    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 22);
    TS_ASSERT_DELTA(outY[3], 0.0, 1e-6);
    TS_ASSERT_DELTA(outY[4], 2.991736, 1e-6);
    TS_ASSERT_DELTA(outY[8], 6.198347, 1e-6);
    TS_ASSERT_DELTA(outY[11], 3.818182, 1e-6);
    TS_ASSERT_DELTA(outY[12], 0.0, 1e-6);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.003692, 1e-6);
  }

  void testLrefGreaterThanLd() {
    // setup
    const MatrixWorkspace_sptr inWS = this->makeFakeWorkspace();
    UnwrapMonitor algo;
    setupAlgorithm(algo, inWS, 17.0);

    // run the algorithm
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS =
        algo.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 44);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.001582, 1e-6);
  }

  void testLrefEqualsLd() {
    // setup
    const MatrixWorkspace_sptr inWS = this->makeFakeWorkspace();
    UnwrapMonitor algo;
    setupAlgorithm(algo, inWS, 15.0);

    // run the algorithm
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS =
        algo.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 49);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.000264, 1e-6);
  }

  void testMinPossibleLref() {
    // setup
    const MatrixWorkspace_sptr inWS = this->makeFakeWorkspace();
    UnwrapMonitor algo;
    setupAlgorithm(algo, inWS, 0.01);

    // run the algorithm
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS =
        algo.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 50);
    TS_ASSERT_DELTA(outY[0], 100.0, 1e-6);
    TS_ASSERT_DELTA(outY[1], 0.0, 1e-6);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.0, 1e-6);
  }

  void testLargeLref() {
    // setup
    const MatrixWorkspace_sptr inWS = this->makeFakeWorkspace();
    UnwrapMonitor algo;
    setupAlgorithm(algo, inWS, 100.0);

    // run the algorithm
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS =
        algo.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 10);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.0, 1e-6);
  }
};

#endif /* MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_ */
