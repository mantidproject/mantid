#ifndef MANTID_CURVEFITTING_CONVERTTOYSPACETEST_H_
#define MANTID_CURVEFITTING_CONVERTTOYSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ConvertToYSpace.h"
#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::ConvertToYSpace;

class ConvertToYSpaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToYSpaceTest *createSuite() {
    return new ConvertToYSpaceTest();
  }
  static void destroySuite(ConvertToYSpaceTest *suite) { delete suite; }

  void test_Init() {
    ConvertToYSpace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  // --------------------------------- Success cases
  // -----------------------------------

  void test_exec_with_TOF_input_gives_correct_X_values() {
    using namespace Mantid::API;

    auto alg = createAlgorithm();
    double x0(50.0), x1(300.0), dx(0.5);
    auto testWS = ComptonProfileTestHelpers::createTestWorkspace(1, x0, x1, dx,
                                                                 true, true);
    alg->setProperty("InputWorkspace", testWS);
    alg->setProperty("Mass", 1.0097);
    alg->setProperty("QWorkspace", "ConvertToYSpace_Test_qSpace");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    // Get the y-Space output workspace
    MatrixWorkspace_sptr ySpOutputWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT(ySpOutputWs != 0)
    TS_ASSERT_EQUALS(testWS->getNumberHistograms(),
                     ySpOutputWs->getNumberHistograms());

    // Get the q-Space output workspace
    MatrixWorkspace_sptr qSpOutputWs = alg->getProperty("QWorkspace");
    TS_ASSERT(qSpOutputWs != 0)
    TS_ASSERT_EQUALS(testWS->getNumberHistograms(),
                     qSpOutputWs->getNumberHistograms());

    const size_t npts = ySpOutputWs->blocksize();

    // Test a few y-Space values
    const auto &ySpOutX = ySpOutputWs->readX(0);
    const auto &ySpOutY = ySpOutputWs->readY(0);
    const auto &ySpOutE = ySpOutputWs->readE(0);

    // X
    TS_ASSERT_DELTA(-18.71348856, ySpOutX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, ySpOutX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, ySpOutX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(-0.01152733, ySpOutY.front(), 1e-08);
    TS_ASSERT_DELTA(5.56667697, ySpOutY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(-0.35141703, ySpOutY.back(), 1e-08);
    // E
    TS_ASSERT_DELTA(25.14204252, ySpOutE.front(), 1e-08);
    TS_ASSERT_DELTA(36.99940026, ySpOutE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(138.38603736, ySpOutE.back(), 1e-08);

    // Test a few q-Space values
    const auto &qSpOutX = qSpOutputWs->readX(0);
    const auto &qSpOutY = qSpOutputWs->readY(0);
    const auto &qSpOutE = qSpOutputWs->readE(0);

    // X
    TS_ASSERT_DELTA(-18.71348856, qSpOutX.front(), 1e-08);
    TS_ASSERT_DELTA(-1.670937938, qSpOutX[npts / 2], 1e-08);
    TS_ASSERT_DELTA(17.99449408, qSpOutX.back(), 1e-08);
    // Y
    TS_ASSERT_DELTA(61.71776650, qSpOutY.front(), 1e-08);
    TS_ASSERT_DELTA(102.09566873, qSpOutY[npts / 2], 1e-08);
    TS_ASSERT_DELTA(524.16435679, qSpOutY.back(), 1e-08);
    // E (in q-Space error is not required)
    TS_ASSERT_DELTA(0.0, qSpOutE.front(), 1e-08);
    TS_ASSERT_DELTA(0.0, qSpOutE[npts / 2], 1e-08);
    TS_ASSERT_DELTA(0.0, qSpOutE.back(), 1e-08);
  }

  // --------------------------------- Failure cases
  // -----------------------------------

  void test_Negative_Or_Zero_Mass_Throws_Error() {
    auto alg = createAlgorithm();

    // Zero
    TS_ASSERT_THROWS(alg->setProperty("Mass", 0.0), std::invalid_argument);
    // Negative
    TS_ASSERT_THROWS(alg->setProperty("Mass", -0.1), std::invalid_argument);
  }

  void test_Input_Workspace_Not_In_TOF_Throws_Error() {
    auto alg = createAlgorithm();
    auto testWS = WorkspaceCreationHelper::Create2DWorkspace123(1, 10);
    testWS->getAxis(0)->setUnit("Wavelength");

    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", testWS),
                     std::invalid_argument);
  }

  void test_Input_Workspace_In_TOF_Without_Instrument_Throws_Error() {
    auto alg = createAlgorithm();
    auto testWS = WorkspaceCreationHelper::Create2DWorkspace123(1, 10);
    testWS->getAxis(0)->setUnit("TOF");

    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", testWS),
                     std::invalid_argument);
  }

  void
  test_Input_Workspace_In_TOF_With_Instrument_But_No_Detector_Parameters_Throws_Error_On_Execution() {
    auto alg = createAlgorithm();
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        1, 10, false, false, false);
    testWS->getAxis(0)->setUnit("TOF");

    alg->setProperty("InputWorkspace", testWS);
    alg->setProperty("Mass", 1.0097);
    alg->setRethrows(true);

    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

private:
  Mantid::API::IAlgorithm_sptr createAlgorithm() {
    Mantid::API::IAlgorithm_sptr alg = boost::make_shared<ConvertToYSpace>();
    alg->initialize();
    alg->setChild(true);
    alg->setPropertyValue("OutputWorkspace", "__UNUSED__");
    return alg;
  }
};

#endif /* MANTID_CURVEFITTING_CONVERTTOYSPACETEST_H_ */
