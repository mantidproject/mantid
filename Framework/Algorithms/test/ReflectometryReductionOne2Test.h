#ifndef ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONE2TEST_H_
#define ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONE2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ReflectometryReductionOne2.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace WorkspaceCreationHelper;

class ReflectometryReductionOne2Test : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_multiDetectorWS;
  MatrixWorkspace_sptr m_wavelengthWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOne2Test *createSuite() {
    return new ReflectometryReductionOne2Test();
  }
  static void destroySuite(ReflectometryReductionOne2Test *suite) {
    delete suite;
  }

  ReflectometryReductionOne2Test() {
    FrameworkManager::Instance();
    // A multi detector ws
    m_multiDetectorWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
    // A workspace in wavelength
    m_wavelengthWS = Create2DWorkspace154(1, 10, true);
    m_wavelengthWS->setInstrument(m_multiDetectorWS->getInstrument());
    m_wavelengthWS->getAxis(0)->setUnit("Wavelength");

  }

  void test_IvsLam() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT(outLam);
    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 8);
    TS_ASSERT(outLam->readX(0)[0] >= 1.5);
    TS_ASSERT(outLam->readX(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 3.1530, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 3.1530, 0.0001);
  }

  void test_IvsLam_processing_instructions_1to2() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 1+2

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1+2");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 8);
    TS_ASSERT(outLam->readX(0)[0] >= 1.5);
    TS_ASSERT(outLam->readX(0)[7] <= 15.0);
    // Y counts, should be 3.1530 * 2
    TS_ASSERT_DELTA(outLam->y(0)[0], 6.3060, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 6.3060, 0.0001);
  }

  void test_IvsLam_processing_instructions_1to3(){
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 1-3

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1-3");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 8);
    TS_ASSERT(outLam->readX(0)[0] >= 1.5);
    TS_ASSERT(outLam->readX(0)[7] <= 15.0);
    // Y counts, should be 3.1530 * 3
    TS_ASSERT_DELTA(outLam->y(0)[0], 9.4590, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 9.4590, 0.0001);
  }

  void xtest_bad_processing_instructions() {
    // Processing instructions : 5+6

    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_multiDetectorWS);
    alg->setProperty("WavelengthMin", 1.5);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setPropertyValue("OutputWorkspace", "IvsQ");
    alg->setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg->setPropertyValue("ProcessingInstructions", "1+2");
    // Must throw as spectrum 2 is not defined
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void xtest_IvsLam_direct_beam() {
    // Test IvsLam workspace
    // No monitor normalization
    // Direct beam normalization: 2-3
    // No transmission correction
    // Processing instructions : 1

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("RegionOfDirectBeam", "2-3");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 8);
    TS_ASSERT(outLam->readX(0)[0] >= 1.5);
    TS_ASSERT(outLam->readX(0)[7] <= 15.0);
    // Y counts, should be 0.5 = 1 (from detector ws) / 2 (from direct beam)
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.5, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.5, 0.0001);
  }

  void xtest_bad_direct_beam() {
    // Direct beam : 4-5

    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_multiDetectorWS);
    alg->setProperty("WavelengthMin", 1.5);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setProperty("MomentumTransferStep", 0.1);
    alg->setProperty("AnalysisMode", "PointDetectorAnalysis");
    alg->setProperty("NormalizeByIntegratedMonitors", "0");
    alg->setPropertyValue("ProcessingInstructions", "1");
    alg->setPropertyValue("OutputWorkspace", "IvsQ");
    alg->setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg->setPropertyValue("RegionOfDirectBeam", "4-5");
    // Must throw region of interest is incompatible with point detector
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_IvsLam_no_monitors() {
    // Test IvsLam workspace
    // No monitor normalization
    // Direct beam normalization: 2-3
    // No transmission correction
    // Processing instructions : 1

    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : Not given
    // MonitorBackgroundWavelengthMax : Not given

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 8);
    TS_ASSERT(outLam->readX(0)[0] >= 1.5);
    TS_ASSERT(outLam->readX(0)[7] <= 15.0);
    // No monitors considered because MonitorBackgroundWavelengthMin
    // and MonitorBackgroundWavelengthMax were not set
    // Y counts must be 3.1530
    TS_ASSERT_DELTA(outLam->y(0)[0], 3.1530, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 3.1530, 0.0001);
  }

  void xtest_IvsLam_monitor_normalization() {
    // Test IvsLam workspace
    // Monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 1

    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // Normalize by integrated monitors : No

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = inputWS->mutableY(0);
    Y = 1.0;
    //std::fill(Y.begin(), Y.begin()+2, 1.0);

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setProperty("NormalizeByIntegratedMonitors", "0");
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 8);
    TS_ASSERT(outLam->readX(0)[0] >= 1.5);
    TS_ASSERT(outLam->readX(0)[7] <= 15.0);
    // Expected values are 2.4996 = 3.15301 (detectors) / 1.26139 (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.4996, 0.0001);
  }

  void xtest_IvsLam_integrated_monitors() {
    // Test IvsLam workspace
    // Monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 1

    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // Normalize by integrated monitors : Yes

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = inputWS->mutableY(0);
    Y = 1.0;
    //std::fill(Y.begin(), Y.begin()+2, 1.0);

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setProperty("NormalizeByIntegratedMonitors", "1");
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 8);
    TS_ASSERT(outLam->readX(0)[0] >= 1.5);
    TS_ASSERT(outLam->readX(0)[7] <= 15.0);
    // Expected values are 0.3124 = 3.15301 (detectors) / (1.26139*8) (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.3124, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.3124, 0.0001);
  }

  void xtest_transmission_correction_run() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("FirstTransmissionRun", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 10);
    // Expected values are 1 = m_wavelength / m_wavelength
    TS_ASSERT_DELTA(outLam->y(0)[0], 1.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 1.0000, 0.0001);
  }

  void xtest_exponential_correction() {
    // CorrectionAlgorithm: ExponentialCorrection
    
    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setProperty("CorrectDetectorPositions", false);
    alg.setProperty("CorrectionAlgorithm", "ExponentialCorrection");
    alg.setProperty("C0", 0.2);
    alg.setProperty("C1", 0.1);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 10);
    // Expected values are 29.0459 and 58.4912
    TS_ASSERT_DELTA(outLam->y(0)[0], 29.0459, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 58.4912, 0.0001);
  }

  void test_polynomial_correction() {
    // CorrectionAlgorithm: PolynomialCorrection

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setProperty("CorrectionAlgorithm", "PolynomialCorrection");
    alg.setProperty("Polynomial", "0.1,0.3,0.5");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 10);
    // Expected values are 2.9851 and 0.1289
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.9851, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.1289, 0.0001);
  }

  void xtest_IvsQ_no_rebin() {

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");

    // TODO: test some values
  }

  void xtest_IvsQ_linear_binning() {

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setProperty("MomentumTransferMin", 1.0);
    alg.setProperty("MomentumTransferMax", 10.0);
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");

    // TODO: choose appropriate values for MomentumTransferMin, MomentumTransferMax, MomentumTransferStep
    // TODO: test some values
  }

  void xtest_IvsQ_logarithmic_binning() {

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setProperty("MomentumTransferMin", 1.0);
    alg.setProperty("MomentumTransferMax", 10.0);
    alg.setProperty("MomentumTransferStep", 0.04);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");

    // TODO: choose appropriate values for MomentumTransferMin, MomentumTransferMax, MomentumTransferStep
    // TODO: test some values
  }

  void xtest_IvsQ_q_range() {

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");

    // TODO: check that q = 4 * pi / lambda * sin(theta)
  }

  void xtest_IvsQ_scale() {

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_wavelengthWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "100");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outQ->blocksize(), 10);

    // TODO: test some values
  }

};

#endif /* ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONE2TEST_H_ */
