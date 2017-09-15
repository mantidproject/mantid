#ifndef MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_
#define MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/Axis.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <string>

#include "MantidAlgorithms/UnwrapMonitor.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class UnwrapMonitorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnwrapMonitorTest *createSuite() { return new UnwrapMonitorTest(); }
  static void destroySuite(UnwrapMonitorTest *suite) { delete suite; }

  void testLrefLessThanLd() {
    // setup and run the algorithm (includes basic checks)
    UnwrapMonitor algo;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(algo, 11.0);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(algo, inWS);

    // specific checks
    const Mantid::MantidVec outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 23);
    TS_ASSERT_DELTA(outX[0], 0.0, 1e-6);
    TS_ASSERT_DELTA(outX[11], 0.008991, 1e-6);
    TS_ASSERT_DELTA(outX[22], 0.017982, 1e-6);

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
    // setup and run the algorithm (includes basic checks)
    UnwrapMonitor algo;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(algo, 17.0);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(algo, inWS);

    // specific checks
    const Mantid::MantidVec outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 45);
    TS_ASSERT_DELTA(outX[0], 0.0, 1e-6);
    TS_ASSERT_DELTA(outX[22], 0.005818, 1e-6);
    TS_ASSERT_DELTA(outX[44], 0.011635, 1e-6);

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 44);
    TS_ASSERT_DELTA(outY[0], 2.005348, 1e-6);
    TS_ASSERT_DELTA(outY[22], 2.005348, 1e-6);
    TS_ASSERT_DELTA(outY[42], 2.005348, 1e-6);
    TS_ASSERT_DELTA(outY[43], 1.770053, 1e-6);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.001582, 1e-6);
  }

  void testLrefEqualsLd() {
    // setup and run the algorithm (includes basic checks)
    UnwrapMonitor algo;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(algo, 15.0);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(algo, inWS);

    // specific checks
    const Mantid::MantidVec outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 50);
    TS_ASSERT_DELTA(outX[0], 0.0, 1e-6);
    TS_ASSERT_DELTA(outX[25], 0.006728, 1e-6);
    TS_ASSERT_DELTA(outX[49], 0.013187, 1e-6);

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 49);
    TS_ASSERT_DELTA(outY[0], 2.040816, 1e-6);
    TS_ASSERT_DELTA(outY[25], 2.040816, 1e-6);
    TS_ASSERT_DELTA(outY[47], 2.040816, 1e-6);
    TS_ASSERT_DELTA(outY[48], 0.040816, 1e-6);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.000264, 1e-6);
  }

  void testMinPossibleLref() {
    // setup and run the algorithm (includes basic checks)
    UnwrapMonitor algo;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(algo, 0.01);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(algo, inWS);

    // specific checks
    const Mantid::MantidVec outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 51);
    TS_ASSERT_DELTA(outX[0], 0.0, 1e-6);
    TS_ASSERT_DELTA(outX[25], 9.890085, 1e-6);
    TS_ASSERT_DELTA(outX[50], 19.780170, 1e-6);

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 50);
    TS_ASSERT_DELTA(outY[0], 100.0, 1e-6);
    TS_ASSERT_DELTA(outY[1], 0.0, 1e-6);
    TS_ASSERT_DELTA(outY[25], 0.0, 1e-6);
    TS_ASSERT_DELTA(outY[49], 0.0, 1e-6);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.0, 1e-6);
  }

  void testLargeLref() {
    // setup and run the algorithm (includes basic checks)
    UnwrapMonitor algo;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(algo, 100.0);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(algo, inWS);

    // specific checks
    const Mantid::MantidVec outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 11);
    TS_ASSERT_DELTA(outX[0], 0.0, 1e-6);
    TS_ASSERT_DELTA(outX[5], 0.000989, 1e-6);
    TS_ASSERT_DELTA(outX[10], 0.001978, 1e-6);

    const Mantid::MantidVec outY = outWS->readY(0);
    TS_ASSERT_EQUALS(outY.size(), 10);
    TS_ASSERT_DELTA(outY[0], 1.5, 1e-6);
    TS_ASSERT_DELTA(outY[5], 1.5, 1e-6);
    TS_ASSERT_DELTA(outY[9], 0.5, 1e-6);

    const double joinWavelength = algo.getProperty("JoinWavelength");
    TS_ASSERT_DELTA(joinWavelength, 0.0, 1e-6);
  }

private:
  const MatrixWorkspace_sptr makeFakeWorkspace() {
    const MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            2, 3, 50);
    testWS->getAxis(0)->setUnit("TOF");
    return testWS;
  }

  // Initialise the algorithm and set the properties. Creates a fake workspace
  // for the input and returns it.
  MatrixWorkspace_const_sptr setupAlgorithm(UnwrapMonitor &algo,
                                            const double lref) {
    // create the workspace
    const MatrixWorkspace_sptr inWS = makeFakeWorkspace();

    // set up the algorithm
    if (!algo.isInitialized())
      algo.initialize();
    algo.setChild(true);
    algo.setProperty("InputWorkspace", inWS);
    algo.setPropertyValue("OutputWorkspace", "outWS");
    algo.setProperty("LRef", lref);

    return inWS;
  }

  // Run the algorithm and do some basic checks. Returns the output workspace.
  MatrixWorkspace_const_sptr
  runAlgorithm(UnwrapMonitor &algo, const MatrixWorkspace_const_sptr inWS) {
    // run the algorithm
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS =
        algo.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    return outWS;
  }
};

#endif /* MANTID_ALGORITHMS_UNWRAPMONITORTEST_H_ */
