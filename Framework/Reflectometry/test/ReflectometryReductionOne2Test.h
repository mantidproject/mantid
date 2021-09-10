// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidReflectometry/ReflectometryReductionOne2.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::HistogramData;
using namespace Mantid::Reflectometry;
using namespace WorkspaceCreationHelper;

namespace {
static constexpr double degToRad = M_PI / 180.;
static constexpr double radToDeg = 180. / M_PI;
} // namespace

class ReflectometryReductionOne2Test : public CxxTest::TestSuite {
private:
  double m_detSize{0.1};
  double m_detPosX{5.0};
  double m_detPosY{5.0};
  // Sample workspace with a monitor and a single detector
  MatrixWorkspace_sptr m_singleDetectorWS;
  // Sample workspace with a monitor and 5 detectors. The detectors are of size
  // m_detSize and the middle one is centred on m_detPosY
  MatrixWorkspace_sptr m_multiDetectorWS;
  // Sample transmission workspace with same detectors as m_multiDetectorWS
  MatrixWorkspace_sptr m_transmissionWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOne2Test *createSuite() { return new ReflectometryReductionOne2Test(); }
  static void destroySuite(ReflectometryReductionOne2Test *suite) { delete suite; }

  ReflectometryReductionOne2Test() {
    FrameworkManager::Instance();
    // A single detector ws
    m_singleDetectorWS = create2DWorkspaceWithReflectometryInstrument(0);
    // A multi detector ws
    m_multiDetectorWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(
        0, m_detSize, V3D(0, 0, 0), V3D(0, 0, 1), 0.5, 1.0, V3D(0, 0, 0), V3D(14, 0, 0), V3D(15, 0, 0), V3D(20, 5, 0),
        6);
    m_multiDetectorWS->mutableRun().addProperty<std::string>("run_number", "1234");
    // A transmission ws with different spectrum numbers to the run
    m_transmissionWS = create2DWorkspaceWithReflectometryInstrumentMultiDetector(
        0, m_detSize, V3D(0, 0, 0), V3D(0, 0, 1), 0.5, 1.0, V3D(0, 0, 0), V3D(14, 0, 0), V3D(15, 0, 0), V3D(20, 5, 0),
        6);
    m_transmissionWS->mutableRun().addProperty<std::string>("run_number", "4321");
    m_transmissionWS->getSpectrum(0).setSpectrumNo(2);
    m_transmissionWS->getSpectrum(1).setSpectrumNo(3);
    m_transmissionWS->getSpectrum(2).setSpectrumNo(4);
    m_transmissionWS->getSpectrum(3).setSpectrumNo(5);
    m_transmissionWS->getSpectrum(4).setSpectrumNo(6);
    m_transmissionWS->getSpectrum(5).setSpectrumNo(7);
    // Set different values in each spectrum so that we can check the correct
    // spectra were used for the transmission correction
    m_transmissionWS->setCounts(0, Counts(m_transmissionWS->y(0).size(), 10));
    m_transmissionWS->setCounts(1, Counts(m_transmissionWS->y(1).size(), 20));
    m_transmissionWS->setCounts(2, Counts(m_transmissionWS->y(2).size(), 20));
    m_transmissionWS->setCounts(3, Counts(m_transmissionWS->y(3).size(), 30));
    m_transmissionWS->setCounts(4, Counts(m_transmissionWS->y(3).size(), 40));
    m_transmissionWS->setCounts(5, Counts(m_transmissionWS->y(3).size(), 40));
  }

  void test_IvsLam() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.0000, 0.0001);
  }

  void test_IvsLam_processing_instructions_3to4() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 3+4

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT(outLam->x(0)[0] >= 1.5);
    TS_ASSERT(outLam->x(0)[7] <= 15.0);
    // Y counts, should be 2.0000 * 2
    TS_ASSERT_DELTA(outLam->y(0)[0], 4.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 4.0000, 0.0001);
  }

  void test_IvsLam_processing_instructions_3to5() {
    // Test IvsLam workspace
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 3-5 spectra is (2-4 workspace indices)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3-5");
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
    // Processing instructions : 4,3+5 (two separate groups)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "4,3+5");
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
    // Processing instructions : 7+8

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "7+8");
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
    setupAlgorithm(alg, 1.5, 15.0, "3");
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
    setupAlgorithm(alg, 1.5, 15.0, "3");
    alg.setProperty("SummationType", "SumInLambda");
    alg.setProperty("ReductionType", "DivergentBeam");
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
    setupAlgorithm(alg, 1.5, 15.0, "3");
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
    // Processing instructions : 3

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
    setupAlgorithmMonitorCorrection(alg, 0.0, 15.0, "4", inputWS, false);
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
    // Processing instructions : 2

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
    setupAlgorithmMonitorCorrection(alg, 0.0, 15.0, "3", inputWS, true);
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
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3", m_multiDetectorWS, false);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    // Expected values are 1 = m_wavelength / m_wavelength
    TS_ASSERT_DELTA(outLam->y(0)[0], 1.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 1.0000, 0.0001);
  }

  void test_transmission_correction_two_runs() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3", m_multiDetectorWS, true);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    // Expected values are 1 = m_wavelength / m_wavelength
    TS_ASSERT_DELTA(outLam->y(0)[0], 1.0000, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 1.0000, 0.0001);
  }

  void test_transmission_correction_with_mapped_spectra() {
    // Run workspace spectrum numbers are 1,2,3,4,5,6.
    // Transmission workspace has spectrum numbers 2,3,4,5,6,7.
    // Spectra with numbers 4-5 exist in both the run and transmission
    // workspaces.
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "4-5", m_transmissionWS, true);
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.0807, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.0802, 0.0001);
  }

  void test_transmission_correction_with_bad_mapped_spectra() {
    // Run workspace spectrum numbers are 1,2,3,4,5,6.
    // Transmission workspace has spectrum numbers 2,3,4,5,6,7.
    // Spectrum 1 exists in the run workspace but not in the transmission
    // workspace.
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "1", m_transmissionWS, true);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_transmission_processing_instructions() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "4-5", m_transmissionWS, false);
    alg.setPropertyValue("TransmissionProcessingInstructions", "4-5");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.0807, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.0802, 0.0001);
  }

  void test_transmission_processing_instructions_with_bad_instructions() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "2-3", m_transmissionWS, false);
    alg.setPropertyValue("TransmissionProcessingInstructions", "1");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_transmission_processing_instructions_that_are_different() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "4-5", m_transmissionWS, false);
    alg.setPropertyValue("TransmissionProcessingInstructions", "4");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.2029, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.2009, 0.0001);
  }

  void test_transmission_processing_instructions_two_runs() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "4", m_transmissionWS, true);
    alg.setPropertyValue("TransmissionProcessingInstructions", "4");
    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg);

    TS_ASSERT_DELTA(outLam->y(0)[0], 0.1009, 0.0001);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.1003, 0.0001);
  }

  void test_exponential_correction() {
    // CorrectionAlgorithm: ExponentialCorrection

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "4");
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
    setupAlgorithm(alg, 1.5, 15.0, "4");
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
    setupAlgorithm(alg, 1.5, 15.0, "4");
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
    // Processing instructions : 3,2+4 (two separate groups)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "4,3+5");
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
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInQ
    // ReductionType : not set (invalid)

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3");
    alg.setProperty("SummationType", "SumInQ");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_sum_in_q_divergent_beam() {
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInQ
    // ReductionType : DivergentBeam
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);

    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 12);
    TS_ASSERT_DELTA(outLam->x(0)[0], 0.934992, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.173599, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.825076, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.768185, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 2.792649, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.787410, 1e-6);

    checkConversionToQ(alg, twoThetaForDetector3() / 2.0);

    TS_ASSERT_DELTA(sumCounts(outLam->counts(0)), 33.310938, 1e-6);
  }

  void test_sum_in_q_non_flat_sample() {
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // SummationType : SumInQ
    // ReductionType : NonFlatSample

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "NonFlatSample");

    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 10);
    TS_ASSERT_DELTA(outLam->x(0)[0], 0.825488, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.064095, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.715573, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 3.141859, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 3.141885, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 3.141920, 1e-6);

    checkConversionToQ(alg, twoThetaForDetector3() / 2.0);

    TS_ASSERT_DELTA(sumCounts(outLam->counts(0)), 31.418985, 1e-6);
  }

  void test_sum_in_q_monitor_normalization() {
    // Monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 3
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
    setupAlgorithmMonitorCorrection(alg, 0.0, 15.0, "4", inputWS, false);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);

    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 13);
    TS_ASSERT_DELTA(outLam->x(0)[0], -0.748672, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[5], 6.315674, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[9], 11.967151, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 5.040302, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[5], 2.193650, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[9], 2.255101, 1e-6);

    checkConversionToQ(alg, twoThetaForDetector4() / 2.0, false);

    TS_ASSERT_DELTA(sumCounts(outLam->counts(0)), 66.514113, 1e-6);
  }

  void test_sum_in_q_transmission_correction_run() {
    // Transmission run is the same as input run

    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3", m_multiDetectorWS, false);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);

    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 12);
    TS_ASSERT_DELTA(outLam->x(0)[0], 0.934992, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.173599, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.825076, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 0.631775, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 0.888541, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 0.886874, 1e-6);

    checkConversionToQ(alg, twoThetaForDetector3() / 2.0, false);

    TS_ASSERT_DELTA(sumCounts(outLam->counts(0)), 21.030473, 1e-6);
  }

  void test_sum_in_q_exponential_correction() {
    // CorrectionAlgorithm: ExponentialCorrection

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "4");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    alg.setProperty("CorrectionAlgorithm", "ExponentialCorrection");
    alg.setProperty("C0", 0.2);
    alg.setProperty("C1", 0.1);

    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 11);
    TS_ASSERT_DELTA(outLam->x(0)[0], 0.920496, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.159104, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.810581, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 16.351599, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 23.963534, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 39.756736, 1e-6);

    checkConversionToQ(alg, twoThetaForDetector4() / 2.0);

    TS_ASSERT_DELTA(sumCounts(outLam->counts(0)), 365.843555, 1e-6);
  }

  void test_sum_in_q_point_detector() {
    // Test IvsQ workspace for a point detector
    // No monitor normalization
    // No direct beam normalization
    // No transmission correction
    // Processing instructions : 1

    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "1");
    alg.setProperty("InputWorkspace", m_singleDetectorWS);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    MatrixWorkspace_sptr outQ = runAlgorithmQ(alg, 28);

    // X range in outQ
    TS_ASSERT_DELTA(outQ->x(0)[0], 0.279882, 1e-6);
    TS_ASSERT_DELTA(outQ->x(0)[3], 0.310524, 1e-6);
    TS_ASSERT_DELTA(outQ->x(0)[7], 0.363599, 1e-6);
    // Y counts
    TS_ASSERT_DELTA(outQ->y(0)[0], 2.900303, 1e-6);
    TS_ASSERT_DELTA(outQ->y(0)[3], 2.886945, 1e-6);
    TS_ASSERT_DELTA(outQ->y(0)[7], 2.607357, 1e-6);

    TS_ASSERT_DELTA(sumCounts(outQ->counts(0)), 79.113420, 1e-6);
  }

  void test_sum_in_q_exclude_partial_bins() {
    // Sum in Q, single detector
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    alg.setProperty("IncludePartialBins", "0");

    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 11);
    TS_ASSERT_DELTA(outLam->x(0)[0], 0.945877, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.184485, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.835962, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 2.767944, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 2.792424, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 2.787199, 1e-6);

    checkConversionToQ(alg, twoThetaForDetector3() / 2.0);

    TS_ASSERT_DELTA(sumCounts(outLam->counts(0)), 30.492737, 1e-6);
  }

  void test_sum_in_q_exclude_partial_bins_multiple_detectors() {
    // Sum in Q, multiple detectors in group
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3-5");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 25.0);
    alg.setProperty("IncludePartialBins", "0");

    MatrixWorkspace_sptr outLam = runAlgorithmLam(alg, 11);
    TS_ASSERT_DELTA(outLam->x(0)[0], 0.957564, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[3], 5.196172, 1e-6);
    TS_ASSERT_DELTA(outLam->x(0)[7], 10.847649, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[0], 8.458467, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[3], 8.521195, 1e-6);
    TS_ASSERT_DELTA(outLam->y(0)[7], 8.306563, 1e-6);

    checkConversionToQ(alg, twoThetaForDetector4() / 2.0);

    TS_ASSERT_DELTA(sumCounts(outLam->counts(0)), 93.056874, 1e-6);
  }

  void test_angle_correction_is_done_for_sum_in_lambda_when_theta_provided() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");

    double const thetaIn = 22.0;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    setYValuesToWorkspace(*inputWS);

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("ThetaIn", thetaIn);
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    checkAngleCorrection(outLam, outQ, thetaIn);
    checkDetector3And4SummedInLambda(outLam, outQ);
  }

  void test_angle_correction_is_not_done_for_sum_in_lambda_when_theta_not_provided() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");

    double const detectorTheta = twoThetaForDetector3And4() / 2.0;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    setYValuesToWorkspace(*inputWS);

    alg.setProperty("InputWorkspace", inputWS);
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    checkAngleCorrection(outLam, outQ, detectorTheta);
    checkDetector3And4SummedInLambda(outLam, outQ);
  }

  void test_angle_correction_not_done_for_single_detector_when_theta_provided() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "4");

    double const detectorTheta = twoThetaForDetector4() / 2.0;
    double const thetaIn = 22.0;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    setYValuesToWorkspace(*inputWS);

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("ThetaIn", thetaIn);
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    checkAngleCorrection(outLam, outQ, detectorTheta);
    checkDetector4SummedInLambda(outLam, outQ);
  }

  void test_angle_correction_not_done_for_single_detector_when_theta_not_provided() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "4");

    double const detectorTheta = twoThetaForDetector4() / 2.0;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    setYValuesToWorkspace(*inputWS);

    alg.setProperty("InputWorkspace", inputWS);
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    checkAngleCorrection(outLam, outQ, detectorTheta);
    checkDetector4SummedInLambda(outLam, outQ);
  }

  void test_requesting_angle_correction_for_sum_in_lambda_throws_for_multiple_groups() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4, 5");
    alg.setProperty("ThetaIn", 22.0);
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }

  void test_requesting_angle_for_sum_in_q_throws_for_multiple_groups() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4, 4");
    alg.setProperty("ThetaIn", 22.0);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
  }

  void test_angle_correction_is_not_done_for_sum_in_q_for_single_detector() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "4");

    double const detectorTheta = twoThetaForDetector4() / 2.0;
    double const thetaIn = 22.0;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    setYValuesToWorkspace(*inputWS);

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("ThetaIn", thetaIn);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    checkAngleCorrection(outLam, outQ, detectorTheta);
    checkDetector4SummedInQ(outLam, outQ);
  }

  void test_angle_correction_is_not_done_for_sum_in_q_for_multiple_detectors() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");

    // The reference angle when summing in Q is taken from the centre of the
    // ROI. If we have an even number of pixels it clips the the lower value,
    // i.e. detector 3 here
    double const detectorTheta = twoThetaForDetector3() / 2.0;
    double const thetaIn = 22.0;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    setYValuesToWorkspace(*inputWS);

    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("ThetaIn", thetaIn);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    checkAngleCorrection(outLam, outQ, detectorTheta);
    checkDetector3And4SummedInQ(outLam, outQ);
  }

  void test_outputs_when_debug_is_false_and_IvsLam_name_not_set() {
    ReflectometryReductionOne2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("Debug", false);
    alg.setPropertyValue("ProcessingInstructions", "3+4");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_outputs_with_default_names_when_debug_is_false_and_run_number_not_set() {
    ReflectometryReductionOne2 alg;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS->mutableRun().removeProperty("run_number");

    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("Debug", false);
    alg.setPropertyValue("ProcessingInstructions", "3+4");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_outputs_with_default_names_when_debug_is_true_and_run_number_not_set() {
    ReflectometryReductionOne2 alg;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS->mutableRun().removeProperty("run_number");

    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("Debug", true);
    alg.setPropertyValue("ProcessingInstructions", "3+4");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam"));

    AnalysisDataService::Instance().clear();
  }

  void test_outputs_with_default_names_when_debug_is_true_and_run_number_is_set() {
    ReflectometryReductionOne2 alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", 1.5);
    alg.setProperty("WavelengthMax", 15.0);
    alg.setProperty("Debug", true);
    alg.setPropertyValue("ProcessingInstructions", "3+4");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsQ_1234"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("IvsLam_1234"));

    AnalysisDataService::Instance().clear();
  }

  void test_transmission_output_is_stored_when_one_transmission_input() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3", m_multiDetectorWS, false);
    runAlgorithmLam(alg);

    TS_ASSERT_EQUALS(alg.getPropertyValue("OutputWorkspaceTransmission"), "TRANS_LAM_1234");

    AnalysisDataService::Instance().clear();
  }

  void test_transmission_output_is_stored_when_two_transmission_inputs() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3", m_multiDetectorWS, true);
    runAlgorithmLam(alg);

    // stitched transmission output is set
    TS_ASSERT_EQUALS(alg.getPropertyValue("OutputWorkspaceTransmission"), "TRANS_LAM_1234_1234");
    // interim transmission outputs are not set
    TS_ASSERT(alg.isDefault("OutputWorkspaceFirstTransmission"));
    TS_ASSERT(alg.isDefault("OutputWorkspaceSecondTransmission"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("TRANS_LAM_1234"));

    AnalysisDataService::Instance().clear();
  }

  void test_background_subtraction_not_done_if_not_enabled_even_if_background_properties_set() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmForBackgroundSubtraction(alg, createWorkspaceWithFlatBackground("test_ws"));
    alg.setProperty("SubtractBackground", false);
    alg.setProperty("BackgroundProcessingInstructions", "2");
    alg.setProperty("BackgroundCalculationMethod", "PerDetectorAverage");
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS,
                          {"ExtractSpectra", "GroupDetectors", "ConvertUnits", "CropWorkspace", "ConvertUnits"});
  }

  void test_background_subtraction_with_default_properties() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmForBackgroundSubtraction(alg, createWorkspaceWithFlatBackground("test_ws"));
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    // Note that ExtractSpectra is not called because the whole workspace is
    // used for the background subtraction
    checkWorkspaceHistory(outputWS, {"ReflectometryBackgroundSubtraction", "GroupDetectors", "ConvertUnits",
                                     "CropWorkspace", "ConvertUnits"});
    checkHistoryAlgorithmProperties(
        outputWS, 1, 0, {{"ProcessingInstructions", ""}, {"BackgroundCalculationMethod", "PerDetectorAverage"}});
  }

  void test_subtract_flat_background() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmForBackgroundSubtraction(alg, createWorkspaceWithFlatBackground("test_ws"));
    alg.setProperty("BackgroundProcessingInstructions", "1, 2, 4, 5");
    alg.setProperty("BackgroundCalculationMethod", "PerDetectorAverage");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    if (!alg.isExecuted())
      return;
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"ExtractSpectra", "ReflectometryBackgroundSubtraction", "GroupDetectors",
                                     "ConvertUnits", "CropWorkspace", "ConvertUnits"});
    checkHistoryAlgorithmProperties(
        outputWS, 1, 1, {{"ProcessingInstructions", "1-2,4-5"}, {"BackgroundCalculationMethod", "PerDetectorAverage"}});
  }

  void test_subtract_polynomial_background() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmForBackgroundSubtraction(alg, createWorkspaceWithPolynomialBackground("test_ws"));
    alg.setProperty("BackgroundProcessingInstructions", "2-5, 7-9");
    alg.setProperty("BackgroundCalculationMethod", "Polynomial");
    alg.setProperty("DegreeOfPolynomial", "2");
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"ExtractSpectra", "ReflectometryBackgroundSubtraction", "GroupDetectors",
                                     "ConvertUnits", "CropWorkspace", "ConvertUnits"});
    checkHistoryAlgorithmProperties(outputWS, 1, 1,
                                    {{"ProcessingInstructions", "2-5,7-9"},
                                     {"BackgroundCalculationMethod", "Polynomial"},
                                     {"DegreeOfPolynomial", "2"}});
  }

  void test_history_for_sum_in_lambda() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");
    alg.setChild(false); // required to get history
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"GroupDetectors", "ConvertUnits", "CropWorkspace", "ConvertUnits"});
  }

  void test_history_for_sum_in_lambda_with_angle_correction() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");
    alg.setProperty("ThetaIn", 22.0);
    alg.setChild(false); // required to get history
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    // Uses RefRoi instead of ConvertUnits
    checkWorkspaceHistory(outputWS, {"GroupDetectors", "ConvertUnits", "CropWorkspace", "RefRoi"});
  }

  void test_history_for_sum_in_lambda_with_monitor_normalisation() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmMonitorCorrection(alg, 1.5, 15.0, "3+4", m_multiDetectorWS, false);
    alg.setChild(false); // required to get history
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS,
                          {"GroupDetectors", "ConvertUnits", "CropWorkspace", "ConvertUnits", "CalculateFlatBackground",
                           "RebinToWorkspace", "Divide", "CropWorkspace", "ConvertUnits"});
  }

  void test_history_for_sum_in_lambda_with_transmission_normalisation() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3+4", m_multiDetectorWS, false);
    alg.setChild(false); // required to get history
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"GroupDetectors", "ConvertUnits", "CropWorkspace", "CreateTransmissionWorkspace",
                                     "RebinToWorkspace", "Divide", "ConvertUnits"});
  }

  void test_history_for_sum_in_q() {
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 22.0);
    alg.setChild(false); // required to get history
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"ConvertUnits", "CropWorkspace", "RefRoi"});
  }

  void test_history_for_sum_in_q_with_monitor_normalisation() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmMonitorCorrection(alg, 1.5, 15.0, "3+4", m_multiDetectorWS, false);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 22.0);
    alg.setChild(false); // required to get history
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"ConvertUnits", "CropWorkspace", "ConvertUnits", "CalculateFlatBackground",
                                     "RebinToWorkspace", "Divide", "CropWorkspace", "RefRoi"});
  }

  void test_history_for_sum_in_q_with_transmission_normalisation() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3+4", m_multiDetectorWS, false);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.setProperty("ThetaIn", 22.0);
    alg.setChild(false); // required to get history
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"ConvertUnits", "CreateTransmissionWorkspace", "RebinToWorkspace", "Divide",
                                     "CropWorkspace", "RefRoi"});
  }

  void test_IvsQ_is_not_distribution_data() {
    // This may not be correct but this behaviour is historic - the output is
    // not distribution data if the input is not distribution
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outQ->isDistribution(), false);
  }

  void test_IvsQ_is_not_distribution_data_when_angle_correction_is_done() {
    // This may not be correct but this behaviour is historic - the output is
    // not distribution data if the input is not distribution. Similar to above
    // but also check the special case where angle correction is done with
    // RefRoi
    ReflectometryReductionOne2 alg;
    setupAlgorithm(alg, 1.5, 15.0, "3+4");
    alg.setProperty("ThetaIn", 22.0);
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outQ->isDistribution(), false);
  }

  void test_IvsQ_is_distribution_data_if_input_is_distribution() {
    ReflectometryReductionOne2 alg;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWS->clone());
    inputWS->setDistribution(true);
    setupAlgorithm(alg, 1.5, 15.0, "3+4");
    alg.setProperty("InputWorkspace", inputWS);
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outQ->isDistribution(), true);
  }

  void test_IvsQ_is_distribution_data_if_normalised_by_monitor() {
    // Monitor correction causes the divided workspace to become
    // distribution data therefore the output is also distribution
    ReflectometryReductionOne2 alg;
    setupAlgorithmMonitorCorrection(alg, 1.5, 15.0, "3+4", m_multiDetectorWS, false);
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outQ->isDistribution(), true);
  }

  void test_IvsQ_is_distribution_data_if_normalised_by_transmission() {
    // Transmission correction causes the divided workspace to become
    // distribution data therefore the output is also distribution
    ReflectometryReductionOne2 alg;
    setupAlgorithmTransmissionCorrection(alg, 1.5, 15.0, "3+4", m_multiDetectorWS, false);
    alg.execute();
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outQ->isDistribution(), true);
  }

  void test_subtract_background_sum_in_q() {
    ReflectometryReductionOne2 alg;
    setupAlgorithmForBackgroundSubtraction(alg, m_multiDetectorWS);
    alg.setProperty("SummationType", "SumInQ");
    alg.setProperty("ReductionType", "DivergentBeam");
    alg.execute();
    auto outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("IvsQ"));
    checkWorkspaceHistory(outputWS, {"ReflectometryBackgroundSubtraction", "ConvertUnits", "CropWorkspace", "RefRoi"});
  }

private:
  // Do standard algorithm setup
  void setupAlgorithm(ReflectometryReductionOne2 &alg, const double wavelengthMin, const double wavelengthMax,
                      const std::string &procInstr) {
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_multiDetectorWS);
    alg.setProperty("WavelengthMin", wavelengthMin);
    alg.setProperty("WavelengthMax", wavelengthMax);
    alg.setPropertyValue("ProcessingInstructions", procInstr);
    alg.setPropertyValue("IncludePartialBins", "1");
    alg.setPropertyValue("OutputWorkspace", "IvsQ");
    alg.setPropertyValue("OutputWorkspaceWavelength", "IvsLam");
  }

  // Do standard algorithm setup for transmission correction
  void setupAlgorithmTransmissionCorrection(ReflectometryReductionOne2 &alg, const double wavelengthMin,
                                            const double wavelengthMax, const std::string &procInstr,
                                            const MatrixWorkspace_sptr &transWS, const bool multiple_runs) {
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
  void setupAlgorithmMonitorCorrection(ReflectometryReductionOne2 &alg, const double wavelengthMin,
                                       const double wavelengthMax, const std::string &procInstr,
                                       const MatrixWorkspace_sptr &inputWS, const bool integrate) {
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

  void setupAlgorithmForBackgroundSubtraction(ReflectometryReductionOne2 &alg, const MatrixWorkspace_sptr &inputWS) {
    setupAlgorithm(alg, 0, 5, "4");
    alg.setChild(false); // required to get history
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("ThetaIn", 0.5);
    alg.setProperty("I0MonitorIndex", 1);
    alg.setProperty("SubtractBackground", true);
  }

  // Do standard algorithm execution and checks and return IvsLam
  MatrixWorkspace_sptr runAlgorithmLam(ReflectometryReductionOne2 &alg, const size_t blocksize = 14,
                                       const size_t nHist = 1) {
    alg.execute();

    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    TS_ASSERT(outLam);
    TS_ASSERT_EQUALS(outLam->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(outLam->blocksize(), blocksize);

    return outLam;
  }

  // Do standard algorithm execution and checks and return IvsQ
  MatrixWorkspace_sptr runAlgorithmQ(ReflectometryReductionOne2 &alg, const size_t blocksize = 14,
                                     const size_t nHist = 1) {
    alg.execute();

    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outQ);
    TS_ASSERT_EQUALS(outQ->getNumberHistograms(), nHist);
    TS_ASSERT_EQUALS(outQ->blocksize(), blocksize);

    return outQ;
  }

  /** Check conversion of x values in a workspace in lambda to a workspace in Q
   * has been done correctly. Optionally also check the counts.
   *
   * @param alg : the algorithm, which has already been executed
   * @param theta : the theta to use in the conversion in degrees
   * @param checkCounts : if true, also check the counts in the bins are the
   * same
   */
  void checkConversionToQ(ReflectometryReductionOne2 &alg, const double theta, const bool checkCounts = true) {
    // Extract arrays for convenience
    MatrixWorkspace_sptr outLam = alg.getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg.getProperty("OutputWorkspace");
    auto const &edgesLam = outLam->binEdges(0);
    auto const &edgesQ = outQ->binEdges(0);
    auto const &countsLam = outLam->counts(0);
    auto const &countsQ = outQ->counts(0);

    // Check lengths match
    TS_ASSERT_EQUALS(edgesLam.size(), edgesQ.size());
    TS_ASSERT_EQUALS(countsLam.size(), countsQ.size());

    // Check converting the lambda value to Q gives the result we got
    auto const nEdges = edgesQ.size();
    auto const factor = 4 * M_PI * std::sin(theta * degToRad);
    for (size_t i = 0; i < nEdges; ++i)
      TS_ASSERT_DELTA(edgesQ[i], factor / edgesLam[nEdges - 1 - i], 1e-6);

    if (checkCounts) {
      // Counts should be the same in matching bins
      auto const nCounts = countsQ.size();
      for (size_t i = 0; i < nCounts; ++i)
        TS_ASSERT_DELTA(countsQ[i], countsLam[nCounts - 1 - i], 1e-6);
    }
  }

  void setYValuesToWorkspace(MatrixWorkspace &ws) {
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      auto &y = ws.mutableY(i);
      auto const iVal = i > 0 ? i - 1 : 0;
      for (size_t j = 0; j < y.size(); ++j) {
        y[j] += double(j + 1) * double(iVal + 1);
      }
    }
  }

  MatrixWorkspace_sptr createWorkspaceWithFlatBackground(std::string const &name) {
    // Create a workspace with a background of 2 and a peak of 5 in the 2nd
    // index
    auto const nspec = 4;
    auto const background = Counts(nspec, 2);
    auto const peak = Counts(nspec, 5);

    CreateSampleWorkspace alg;
    alg.initialize();
    alg.setChild(false);
    alg.setProperty("NumBanks", nspec + 1);
    alg.setProperty("BankPixelWidth", 1);
    alg.setProperty("XMin", 1.0);
    alg.setProperty("XMax", 5.0);
    alg.setProperty("BinWidth", 1.0);
    alg.setProperty("XUnit", "TOF");
    alg.setProperty("WorkspaceType", "Histogram");
    alg.setProperty("NumMonitors", 0);
    alg.setPropertyValue("OutputWorkspace", name);
    alg.execute();

    auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(name));
    ws->setCounts(0, background);
    ws->setCounts(1, background);
    ws->setCounts(2, peak);
    ws->setCounts(3, background);
    ws->setCounts(4, background);
    return ws;
  }

  MatrixWorkspace_sptr createWorkspaceWithPolynomialBackground(std::string const &name) {
    // Create a workspace with a polynomial background of degree 2 and a peak
    // of 5 in the 5th spectra
    auto const nspec = 9;
    auto const polynomial = std::vector<int>{1, 8, 13, 16, 17, 16, 13, 8, 1};
    auto const peak = std::vector<int>{0, 0, 0, 0, 5, 0, 0, 0, 0};

    CreateSampleWorkspace alg;
    alg.initialize();
    alg.setChild(false);
    alg.setProperty("NumBanks", nspec);
    alg.setProperty("BankPixelWidth", 1);
    alg.setProperty("XMin", 1.0);
    alg.setProperty("XMax", 2.0);
    alg.setProperty("BinWidth", 1.0);
    alg.setProperty("XUnit", "TOF");
    alg.setProperty("WorkspaceType", "Histogram");
    alg.setProperty("NumMonitors", 0);
    alg.setProperty("OutputWorkspace", name);
    alg.execute();

    auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(name));
    for (auto spec = 0; spec < nspec; ++spec)
      ws->setCounts(spec, Counts(1, polynomial[spec] + peak[spec]));
    return ws;
  }

  // Get twoTheta for detector 4 in m_multiDetectorWS, in degrees
  double twoThetaForDetector4() {
    // Detector 4 is the centre pixel at m_detPosY
    return std::atan(m_detPosY / m_detPosX) * radToDeg;
  }

  // Get twoTheta for detector 3 in m_multiDetectorWS, in degrees
  double twoThetaForDetector3() {
    // One below the centre pixel
    return std::atan((m_detPosY - m_detSize) / m_detPosX) * radToDeg;
  }

  // Get the average of the twoTheta's of detectors 3 and 4 for
  // m_multiDetectorWS, in degrees. This is the same as the twoTheta that
  // DetectorInfo will return if these detectors are grouped/summed into a
  // single spectrum.
  double twoThetaForDetector3And4() { return (twoThetaForDetector4() + twoThetaForDetector3()) / 2.0; }

  void checkWorkspaceHistory(const MatrixWorkspace_sptr &ws, std::vector<std::string> const &expected,
                             bool const unroll = true) {
    auto wsHistory = ws->getHistory();
    auto algHistories = wsHistory.getAlgorithmHistories();
    auto algNames = std::vector<std::string>();
    if (unroll && algHistories.size() > 0) {
      auto lastAlgHistory = algHistories.back();
      auto childHistories = lastAlgHistory->getChildHistories();
      std::transform(childHistories.cbegin(), childHistories.cend(), std::back_inserter(algNames),
                     [](const AlgorithmHistory_const_sptr &childAlg) { return childAlg->name(); });
    } else if (!unroll) {
      std::transform(algHistories.cbegin(), algHistories.cend(), std::back_inserter(algNames),
                     [](const AlgorithmHistory_sptr &alg) { return alg->name(); });
    }
    TS_ASSERT_EQUALS(algNames, expected);
  }

  void checkHistoryAlgorithmProperties(const MatrixWorkspace_sptr &ws, size_t toplevelIdx, size_t childIdx,
                                       std::map<std::string, std::string> const &expected) {
    auto parentHist = ws->getHistory().getAlgorithmHistory(toplevelIdx);
    auto childHistories = parentHist->getChildHistories();
    TS_ASSERT(childHistories.size() > childIdx);
    if (childIdx >= childHistories.size())
      return;
    auto childHist = childHistories[childIdx];
    for (auto kvp : expected)
      TS_ASSERT_EQUALS(childHist->getPropertyValue(kvp.first), kvp.second);
  }

  void checkAngleCorrection(MatrixWorkspace_sptr outLam, MatrixWorkspace_sptr outQ, double const theta) {
    auto const &qX = outQ->x(0);
    auto const &lamX = outLam->x(0);

    std::vector<double> lamXinv(lamX.size() + 3);
    std::reverse_copy(lamX.begin(), lamX.end(), lamXinv.begin());

    auto factor = 4.0 * M_PI * sin(theta * degToRad);
    for (size_t i = 0; i < qX.size(); ++i) {
      TS_ASSERT_DELTA(qX[i], factor / lamXinv[i], 1e-6);
    }
  }

  void checkDetector3And4SummedInLambda(MatrixWorkspace_sptr outLam, MatrixWorkspace_sptr outQ, int const wsIdx = 0) {
    auto const &lamY = outLam->y(wsIdx);
    TS_ASSERT_EQUALS(lamY.size(), 14);
    TS_ASSERT_DELTA(lamY[0], 19, 1e-2);
    TS_ASSERT_DELTA(lamY[6], 49, 1e-2);
    TS_ASSERT_DELTA(lamY[13], 84, 1e-2);

    auto const &qY = outQ->y(wsIdx);
    TS_ASSERT_EQUALS(qY.size(), 14);
    TS_ASSERT_DELTA(qY[0], 84, 1e-2);
    TS_ASSERT_DELTA(qY[6], 54, 1e-2);
    TS_ASSERT_DELTA(qY[13], 19, 1e-2);
  }

  void checkDetector4SummedInLambda(MatrixWorkspace_sptr outLam, MatrixWorkspace_sptr outQ, int const wsIdx = 0) {
    auto const &lamY = outLam->y(wsIdx);
    TS_ASSERT_EQUALS(lamY.size(), 14);
    TS_ASSERT_DELTA(lamY[0], 11, 1e-2);
    TS_ASSERT_DELTA(lamY[6], 29, 1e-2);
    TS_ASSERT_DELTA(lamY[13], 50, 1e-2);

    auto const &qY = outQ->y(wsIdx);
    TS_ASSERT_EQUALS(qY.size(), 14);
    TS_ASSERT_DELTA(qY[0], 50, 1e-2);
    TS_ASSERT_DELTA(qY[6], 32, 1e-2);
    TS_ASSERT_DELTA(qY[13], 11, 1e-2);
  }

  void checkDetector4SummedInQ(MatrixWorkspace_sptr outLam, MatrixWorkspace_sptr outQ, int const wsIdx = 0) {
    auto const &lamY = outLam->y(wsIdx);
    TS_ASSERT_EQUALS(lamY.size(), 10);
    TS_ASSERT_DELTA(lamY[0], 13.954514, 1e-6);
    TS_ASSERT_DELTA(lamY[6], 60.379735, 1e-6);
    TS_ASSERT_DELTA(lamY[9], 83.408536, 1e-6);

    auto const &qY = outQ->y(wsIdx);
    TS_ASSERT_EQUALS(qY.size(), 10);
    TS_ASSERT_DELTA(qY[0], 83.408536, 1e-6);
    TS_ASSERT_DELTA(qY[6], 37.016271, 1e-6);
    TS_ASSERT_DELTA(qY[9], 13.954514, 1e-6);
  }

  void checkDetector4SummedInQCroppedToDetector3And4(MatrixWorkspace_sptr outLam, MatrixWorkspace_sptr outQ,
                                                     int const wsIdx = 0) {
    auto const &lamY = outLam->y(wsIdx);
    TS_ASSERT_EQUALS(lamY.size(), 10);
    TS_ASSERT_DELTA(lamY[0], 13.906629, 1e-6);
    TS_ASSERT_DELTA(lamY[6], 60.329214, 1e-6);
    TS_ASSERT_DELTA(lamY[9], 83.364379, 1e-6);

    auto const &qY = outQ->y(wsIdx);
    TS_ASSERT_EQUALS(qY.size(), 10);
    TS_ASSERT_DELTA(qY[0], 83.3643785180, 1e-6);
    TS_ASSERT_DELTA(qY[6], 36.9686340548, 1e-6);
    TS_ASSERT_DELTA(qY[9], 13.9066291140, 1e-6);
  }

  void checkDetector3And4SummedInQ(MatrixWorkspace_sptr outLam, MatrixWorkspace_sptr outQ, int const wsIdx = 0) {
    auto const &lamY = outLam->y(wsIdx);
    TS_ASSERT_EQUALS(lamY.size(), 10);
    TS_ASSERT_DELTA(lamY[0], 24.275146, 1e-6);
    TS_ASSERT_DELTA(lamY[6], 101.852986, 1e-6);
    TS_ASSERT_DELTA(lamY[9], 140.267317, 1e-6);

    auto const &qY = outQ->y(wsIdx);
    TS_ASSERT_EQUALS(qY.size(), 10);
    TS_ASSERT_DELTA(qY[0], 140.267317, 1e-6);
    TS_ASSERT_DELTA(qY[6], 62.816137, 1e-6);
    TS_ASSERT_DELTA(qY[9], 24.275146, 1e-6);
  }

  double sumCounts(Mantid::HistogramData::Counts const &counts) {
    return std::accumulate(counts.cbegin(), counts.cend(), 0.0);
  }
};
