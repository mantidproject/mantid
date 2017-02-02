#ifndef ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACE2TEST_H_
#define ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACE2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CreateTransmissionWorkspace2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <algorithm>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace WorkspaceCreationHelper;

class CreateTransmissionWorkspace2Test : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_multiDetectorWS;
  MatrixWorkspace_sptr m_wavelengthWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspace2Test *createSuite() {
    return new CreateTransmissionWorkspace2Test();
  }
  static void destroySuite(CreateTransmissionWorkspace2Test *suite) {
    delete suite;
  }

  CreateTransmissionWorkspace2Test() {
    FrameworkManager::Instance();
    // A multi detector ws
    m_multiDetectorWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
    // A workspace in wavelength
    m_wavelengthWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
    m_wavelengthWS->getAxis(0)->setUnit("Wavelength");
  }

  void test_execute() {
    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_trans_run_in_wavelength_throws() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_ANYTHING(
        alg.setProperty("FirstTransmissionRun", m_wavelengthWS));
    TS_ASSERT_THROWS_ANYTHING(
        alg.setProperty("SecondTransmissionRun", m_wavelengthWS));
  }

  void test_wavelength_min_is_mandatory() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_wavelength_max_is_mandatory() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_processing_instructions_is_mandatory() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_wavelength_range() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("WavelengthMin", 15.0);
    alg.setProperty("WavelengthMax", 1.5);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_range() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("MonitorBackgroundWavelengthMin", 15.0);
    alg.setProperty("MonitorBackgroundWavelengthMax", 10.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_bad_monitor_integration_range() {

    CreateTransmissionWorkspace2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("ProcessingInstructions", "1");
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.0);
    alg.setProperty("MonitorIntegrationWavelengthMax", 0.0);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_one_tranmission_run() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outLam);
    TS_ASSERT_EQUALS("Wavelength", outLam->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
  }

  void test_one_run_processing_instructions() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1+2");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outLam);
    TS_ASSERT_EQUALS("Wavelength", outLam->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Y counts, should be 2.0000 * 2
    TS_ASSERT_DELTA(outLam->y(0)[0], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 4.0000, 0.0001);
  }

  void test_one_run_monitor_normalization() {
    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // Normalize by integrated monitors : No

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = inputWS->mutableY(0);
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 0.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 10);
    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 2.4996 = 3.15301 (detectors) / 1.26139 (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[2], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[4], 2.4996, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.4996, 0.0001);
  }

  void test_one_run_integrated_monitor_normalization() {
    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // Normalize by integrated monitors : No

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = inputWS->mutableY(0);
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", inputWS);
    alg.setProperty("WavelengthMin", 0.0);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    alg.setProperty("MonitorIntegrationWavelengthMin", 1.5);
    alg.setProperty("MonitorIntegrationWavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 16);
    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 0.1981 = 2.0000 (detectors) / (1.26139*8) (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.1981, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.1981, 0.0001);
  }

  void test_two_transmission_runs() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("SecondTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 14);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
  }

  void test_two_transmission_runs_stitch_params() {

    CreateTransmissionWorkspace2 alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("FirstTransmissionRun", m_multiDetectorWS);
    alg.setProperty("SecondTransmissionRun", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("Params", "0.1");
    alg.setPropertyValue("ProcessingInstructions", "1");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outLam->blocksize(), 126);
    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->x(0)[0], 1.7924, 0.0001);
    TS_ASSERT_DELTA(outLam->x(0)[1], 1.8924, 0.0001);
    TS_ASSERT_DELTA(outLam->x(0)[2], 1.9924, 0.0001);
    TS_ASSERT_DELTA(outLam->x(0)[3], 2.0924, 0.0001);
  }
};

#endif /* ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACE2TEST_H_ */
