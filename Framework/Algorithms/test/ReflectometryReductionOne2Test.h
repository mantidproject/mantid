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
    m_wavelengthWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
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
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
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
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Y counts, should be 2.0000 * 2
    TS_ASSERT_DELTA(outLam->y(0)[0], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 4.0000, 0.0001);
  }

  void test_IvsLam_processing_instructions_1to3() {
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
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Y counts, should be 2.0000 * 3
    TS_ASSERT_DELTA(outLam->y(0)[0], 6.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 6.0000, 0.0001);
  }

  void test_bad_processing_instructions() {
    // Processing instructions : 5+6

    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_multiDetectorWS);
    alg->setProperty("WavelengthMin", 1.5);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setPropertyValue("OutputWorkspace", "IvsQ");
    alg->setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg->setPropertyValue("ProcessingInstructions", "5+6");
    // Must throw as spectrum 2 is not defined
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
  }

  void test_IvsLam_direct_beam() {
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
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    // Y counts, should be 0.5 = 1 (from detector ws) / 2 (from direct beam)
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.5, 0.0001);
  }

  void test_bad_direct_beam() {
    // Direct beam : 4-5

    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_multiDetectorWS);
    alg->setProperty("WavelengthMin", 1.5);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setPropertyValue("ProcessingInstructions", "1");
    alg->setPropertyValue("OutputWorkspace", "IvsQ");
    alg->setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg->setPropertyValue("RegionOfDirectBeam", "4-5");
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
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // No monitors considered because MonitorBackgroundWavelengthMin
    // and MonitorBackgroundWavelengthMax were not set
    // Y counts must be 2.0000
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
  }

  void test_IvsLam_monitor_normalization() {
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
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("WavelengthMin", 0.0);
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
    TS_ASSERT_EQUALS(outLam->blocksize(), 10);
    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 2.4996 = 3.15301 (detectors) / 1.26139 (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[2], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[4], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.4996, 0.0001);
  }

  void test_IvsLam_integrated_monitors() {
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
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("WavelengthMin", 0.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setProperty("NormalizeByIntegratedMonitors", "1");
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.5);
    alg.setProperty("MonitorIntegrationWavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 16);
    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 0.1981 = 2.0000 (detectors) / (1.26139*8) (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.1981, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.1981, 0.0001);
  }

  void test_transmission_correction_run() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    // Expected values are 1 = m_wavelength / m_wavelength
    TS_ASSERT_DELTA(outLam->y(0)[0], 1.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 1.0000, 0.0001);
  }

  void test_transmission_correction_two_runs() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("SecondTransmissionRun", m_multiDetectorWS);
    alg.setProperty("StartOverlap", 2.5);
    alg.setProperty("EndOverlap", 3.0);
    alg.setProperty("Params", "0.1");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    // Expected values are 1 = m_wavelength / m_wavelength
    TS_ASSERT_DELTA(outLam->y(0)[0], 1.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 1.0000, 0.0001);
  }

  void test_exponential_correction() {
    // CorrectionAlgorithm: ExponentialCorrection

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("CorrectionAlgorithm", "ExponentialCorrection");
    alg.setProperty("C0", 0.2);
    alg.setProperty("C1", 0.1);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT_DELTA(outLam->y(0)[0], 12.5113, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 23.4290, 0.0001);
  }

  void test_polynomial_correction() {
    // CorrectionAlgorithm: PolynomialCorrection

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("CorrectionAlgorithm", "PolynomialCorrection");
    alg.setProperty("Polynomial", "0.1,0.3,0.5");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.6093, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.0514, 0.0001);
  }

  void test_IvsQ() {

    ReflectometryReductionOne2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outQ->blocksize(), 14);
    // X range in outQ
    TS_ASSERT_DELTA(outQ->x(0)[0], 0.3353, 0.0001);
    TS_ASSERT_DELTA(outQ->x(0)[7], 0.5962, 0.0001);
  }
};

#endif /* ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONE2TEST_H_ */
