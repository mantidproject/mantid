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
  MatrixWorkspace_sptr m_transmissionWS;

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
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    // A transmission ws with different spectrum numbers to the run
    m_transmissionWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    m_transmissionWS->getSpectrum(0).setSpectrumNo(2);
    m_transmissionWS->getSpectrum(1).setSpectrumNo(3);
    m_transmissionWS->getSpectrum(2).setSpectrumNo(4);
    m_transmissionWS->getSpectrum(3).setSpectrumNo(5);
    // Set different values in each spectrum so that we can check the correct
    // spectra were used for the transmission correction
    using namespace Mantid::HistogramData;
    m_transmissionWS->setCounts(0, Counts(m_transmissionWS->y(0).size(), 10));
    m_transmissionWS->setCounts(1, Counts(m_transmissionWS->y(1).size(), 20));
    m_transmissionWS->setCounts(2, Counts(m_transmissionWS->y(2).size(), 30));
    m_transmissionWS->setCounts(3, Counts(m_transmissionWS->y(3).size(), 40));
  }

  void test_IvsLam() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

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
    setupAlgorithm(alg, 1.5, 15.0, "1+2");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

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
    setupAlgorithm(alg, 1.5, 15.0, "1-3");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Y counts, should be 2.0000 * 3
    TS_ASSERT_DELTA(outLam->y(0)[0], 6.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 6.0000, 0.0001);
  }

  void test_IvsLam_multiple_detector_groups() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 2,1+3 (two separate groups)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2,1+3");
    // Run the algorithm. There should be 2 output histograms, one for each
    // input group. Note that the group order is swapped from the input order
    // because they are sorted by the first spectrum number in the group,
    // i.e. as if the input was "1+3,2"
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 14, 2);

    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT(outLam->x(1)[0] >= 1.5);
    TS_ASSERT(outLam->x(1)[7] <= 15.0);
    // Y counts, should be 2.0000 * 2 for first group, 2.0000 * 1 for second.
    TS_ASSERT_DELTA(outLam->y(0)[0], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(1)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(1)[7], 2.0000, 0.0001);
  }

  void test_bad_processing_instructions() {
    // Processing instructions : 5+6

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "5+6");
    // Must throw as spectrum 2 is not defined
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_sum_in_lambda() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInLambda (same as default)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setProperty("SummationType", "SumInLambda");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
  }

  void test_sum_in_lambda_with_bad_reduction_type() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInLambda (same as default)
    // ReductionType : DivergentBeam (invalid)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setProperty("SummationType", "SumInLambda");
    alg.setProperty("ReductionType", "DivergentBeam");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_IvsLam_direct_beam() {
    // Test IvsLam workspace
    // No monitor normalization
    // Direct beam normalization: 2-3
    // No transmission correction
    // Processing instructions : 2

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2");
    alg.setPropertyValue("RegionOfDirectBeam", "2-3");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.4991, 0.0001);
  }

  void test_bad_direct_beam() {
    // Direct beam : 4-5
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setPropertyValue("RegionOfDirectBeam", "4-5");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
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
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setProperty("I0MonitorIndex", "0");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

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
    // Processing instructions : 2

    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // Normalize by integrated monitors : No

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = m_multiDetectorWS->mutableY(0);
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    ReflectometryReductionOne2 alg;
    setupAlgorithmMonitorCorrection(alg, 0.0, 15.0, "2", inputWS, false);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 10);

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
    setupAlgorithmMonitorCorrection(alg, 0.0, 15.0, "1", inputWS, true);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 16);

    TS_ASSERT(outLam->x(0)[0] >= 0.0);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Expected values are 0.1981 = 2.0000 (detectors) / (1.26139*8) (monitors)
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.1981, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.1981, 0.0001);
  }

  void test_transmission_correction_run() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "1", m_multiDetectorWS,
                                         false);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    // Expected values are 1 = m_wavelength / m_wavelength
    TS_ASSERT_DELTA(outLam->y(0)[0], 1.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 1.0000, 0.0001);
  }

  void test_transmission_correction_two_runs() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "1", m_multiDetectorWS,
                                         true);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    // Expected values are 1 = m_wavelength / m_wavelength
    TS_ASSERT_DELTA(outLam->y(0)[0], 1.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 1.0000, 0.0001);
  }

  void test_transmission_correction_with_mapped_spectra() {
    // Run workspace spectrum numbers are 1,2,3,4.
    // Transmission workspace has spectrum numbers 2,3,4,5.
    // Processing instructions 2-3 in the run workspace map to
    // spectra 3-4, which map to indices 1-2 in the transmission
    // workspace.
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "2-3",
                                         m_transmissionWS, true);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.0807, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.0802, 0.0001);
  }

  void test_transmission_correction_with_bad_mapped_spectra() {
    // Run workspace spectrum numbers are 1,2,3,4.
    // Transmission workspace has spectrum numbers 2,3,4,5.
    // Processing instructions 0 in the run workspace maps to
    // spectrum 1, which doesn't exist in the transmission
    // workspace.
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "0", m_transmissionWS,
                                         true);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_transmission_correction_with_different_spectra() {
    // Run workspace spectrum numbers are 1,2,3,4.  Transmission workspace has
    // spectrum numbers 2,3,4,5.  Processing instructions 2,3 are used in the
    // run and transmission workspaces without any mapping i.e. spectra 3-4 in
    // the run and spectra 4-5 in the transmission workspace are used.
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "2-3",
                                         m_transmissionWS, true);
    alg.setProperty("StrictSpectrumChecking", "0");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.0571, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.0571, 0.0001);
  }

  void test_exponential_correction() {
    // CorrectionAlgorithm: ExponentialCorrection

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2");
    alg.setProperty("CorrectionAlgorithm", "ExponentialCorrection");
    alg.setProperty("C0", 0.2);
    alg.setProperty("C1", 0.1);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 12.5113, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 23.4290, 0.0001);
  }

  void test_polynomial_correction() {
    // CorrectionAlgorithm: PolynomialCorrection

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2");
    alg.setProperty("CorrectionAlgorithm", "PolynomialCorrection");
    alg.setProperty("Polynomial", "0.1,0.3,0.5");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.6093, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.0514, 0.0001);
  }

  void test_IvsQ() {
    // Test IvsQ workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 2

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2");
    MatrixWorkspace_sptr outQ = runAlgorithmQ(alg);

    // X range in outQ
    TS_ASSERT_DELTA(outQ->x(0)[0], 0.3353, 0.0001);
    TS_ASSERT_DELTA(outQ->x(0)[7], 0.5962, 0.0001);
    // Y counts
    TS_ASSERT_DELTA(outQ->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outQ->y(0)[7], 2.0000, 0.0001);
  }

  void test_IvsQ_multiple_detector_groups() {
    // Test IvsQ workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 2,1+3 (two separate groups)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2,1+3");
    // Run the algorithm. There should be 2 output histograms, one for each
    // input group. Note that the group order is swapped from the input order
    // because they are sorted by the first spectrum number in the group,
    // i.e. as if the input was "1+3,2"
    MatrixWorkspace_sptr outQ = runAlgorithmQ(alg, 14, 2);

    // X range in outQ
    TS_ASSERT_DELTA(outQ->x(0)[0], 0.3353, 0.0001);
    TS_ASSERT_DELTA(outQ->x(0)[7], 0.5961, 0.0001);
    TS_ASSERT_DELTA(outQ->x(1)[0], 0.3353, 0.0001);
    TS_ASSERT_DELTA(outQ->x(1)[7], 0.5962, 0.0001);
    // Y counts, should be 2.0000 * 2 for first group, 2.0000 * 1 for second.
    TS_ASSERT_DELTA(outQ->y(0)[0], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outQ->y(0)[7], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outQ->y(1)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outQ->y(1)[7], 2.0000, 0.0001);
  }

  void test_sum_in_q_with_bad_reduction_type() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInQ
    // ReductionType : not set (invalid)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setProperty("SummationType", "SumInQ");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_sum_in_q_divergent_beam() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInQ
    // ReductionType : DivergentBeam
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 12);

    TS_ASSERT_DELTA(outLam->x(0)[0], 0.927132, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.165740, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.817217, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.773699, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 2.828460, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.816935, 1e-6);
  }

  void test_sum_in_q_non_flat_sample() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInQ
    // ReductionType : NonFlatSample

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "NonFlatSample");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 10);

    TS_ASSERT_DELTA(outLam->x(0)[0], 0.822974, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.061582, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.713059, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 3.140302, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 3.140457, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 3.140644, 1e-6);
  }

  void test_sum_in_q_direct_beam() {
    // Test IvsLam workspace
    // No monitor normalization
    // Direct beam normalization: 2-3
    // No transmission correction
    // Processing instructions : 2

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2");
    alg.setPropertyValue("RegionOfDirectBeam", "2-3");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 11);

    TS_ASSERT_DELTA(outLam->x(0)[0], 0.913144, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.151752, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.803229, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.447237, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 0.454605, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.451946, 1e-6);
  }

  void test_sum_in_q_monitor_normalization() {
    // Test IvsLam workspace
    // Monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 2
    // SummationType : SumInQ
    // ReductionType : DivergentBeam

    // I0MonitorIndex: 0
    // MonitorBackgroundWavelengthMin : 0.5
    // MonitorBackgroundWavelengthMax : 3.0
    // Normalize by integrated monitors : No

    // Modify counts in monitor (only for this test)
    // Modify counts only for range that will be fitted
    auto inputWS = m_multiDetectorWS;
    auto &Y = m_multiDetectorWS->mutableY(0);
    std::fill(Y.begin(), Y.begin() + 2, 1.0);

    ReflectometryReductionOne2 alg;
    setupAlgorithmMonitorCorrection(alg, 0.0, 15.0, "2", inputWS, false);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 13);

    TS_ASSERT_DELTA(outLam->x(0)[0], -0.742692, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[5], 6.321654, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[9], 11.973131, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 5.044175, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[5], 2.118472, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[9], 2.280546, 1e-6);
  }

  void test_sum_in_q_transmission_correction_run() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "1", m_multiDetectorWS,
                                         false);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 12);

    TS_ASSERT_DELTA(outLam->x(0)[0], 0.927132, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.165740, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.817217, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.620714, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 0.899935, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.896268, 1e-6);
  }

  void test_sum_in_q_exponential_correction() {
    // CorrectionAlgorithm: ExponentialCorrection

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    alg.setProperty("CorrectionAlgorithm", "ExponentialCorrection");
    alg.setProperty("C0", 0.2);
    alg.setProperty("C1", 0.1);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 11);

    TS_ASSERT_DELTA(outLam->x(0)[0], 0.913144, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.151752, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.803229, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 16.353662, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 24.261270, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 39.844321, 1e-6);
  }

  void test_sum_in_q_IvsQ() {
    // Test IvsQ workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 2

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "2");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    MatrixWorkspace_sptr outQ = runAlgorithmQ(alg, 11);

    // X range in outQ
    TS_ASSERT_DELTA(outQ->x(0)[0], 0.292253, 1e-6);
    TS_ASSERT_DELTA(outQ->x(0)[3], 0.393656, 1e-6);
    TS_ASSERT_DELTA(outQ->x(0)[7], 0.732554, 1e-6);
    // Y counts
    TS_ASSERT_DELTA(outQ->y(0)[0], 2.891639, 1e-6);
    TS_ASSERT_DELTA(outQ->y(0)[3], 2.854571, 1e-6);
    TS_ASSERT_DELTA(outQ->y(0)[7], 2.871364, 1e-6);
  }

private:
  // Do standard algorithm setup
  void setupAlgorithm(ReflectometryReductionOne2 &alg,
                      const double wavelengthMin, const double wavelengthMax,
                      const std::string &procInstr) {
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", wavelengthMin);
    alg.setProperty("WavelengthMax", wavelengthMax);
    alg.setPropertyValue("ProcessingInstructions", procInstr);
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
  }

  // Do standard algorithm setup for transmission correction
  void setupAlgorithmTransmissionCorrection(ReflectometryReductionOne2 &alg,
                                            const double wavelengthMin,
                                            const double wavelengthMax,
                                            const std::string &procInstr,
                                            MatrixWorkspace_sptr transWS,
                                            const bool multiple_runs) {
    setupAlgorithm(alg, wavelengthMin, wavelengthMax, procInstr);
    alg.setProperty("FirstTransmissionRun", transWS);
    if (multiple_runs) {
      alg.setProperty("SecondTransmissionRun", transWS);
      alg.setProperty("StartOverlap", 2.5);
      alg.setProperty("EndOverlap", 3.0);
      alg.setProperty("Params", "0.1");
    }
  }

  // Do standard algorithm setup for monitor correction
  void setupAlgorithmMonitorCorrection(ReflectometryReductionOne2 &alg,
                                       const double wavelengthMin,
                                       const double wavelengthMax,
                                       const std::string &procInstr,
                                       MatrixWorkspace_sptr inputWS,
                                       const bool integrate) {
    setupAlgorithm(alg, wavelengthMin, wavelengthMax, procInstr);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("I0MonitorIndex", "0");
    alg.setProperty("MonitorBackgroundWavelengthMin", 0.5);
    alg.setProperty("MonitorBackgroundWavelengthMax", 3.0);
    if (integrate) {
      alg.setProperty("NormalizeByIntegratedMonitors", "1");
      alg.setProperty("MonitorIntegrationWavelengthMin", 1.5);
      alg.setProperty("MonitorIntegrationWavelengthMax", 15.0);
    } else {
      alg.setProperty("NormalizeByIntegratedMonitors", "0");
    }
  }

  // Do standard algorithm execution and checks and return IvsLam
  MatrixWorkspace_sptr runAlgorithmLam(ReflectometryReductionOne2 &alg,
                                       const size_t blocksize = 14,
                                       const size_t nHist = 1) {
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    TS_ASSERT(outLam);
    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(outLam->blocksize(), blocksize);

    return outLam;
  }

  // Do standard algorithm execution and checks and return IvsQ
  MatrixWorkspace_sptr runAlgorithmQ(ReflectometryReductionOne2 &alg,
                                     const size_t blocksize = 14,
                                     const size_t nHist = 1) {
    alg.execute();

    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outQ);
    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(outQ->blocksize(), blocksize);

    return outQ;
  }
};

#endif /* ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONE2TEST_H_ */
