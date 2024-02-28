// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidReflectometry/SpecularReflectionPositionCorrect2.h"
#include <cxxtest/TestSuite.h>

using Mantid::Reflectometry::SpecularReflectionPositionCorrect2;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class SpecularReflectionPositionCorrect2Test : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_d17WS;
  MatrixWorkspace_sptr m_figaroWS;
  MatrixWorkspace_sptr m_interWS;

  static constexpr double radToDeg = 180. / M_PI;
  static constexpr double degToRad = M_PI / 180.;

  // Initialise the algorithm and set the properties
  static void setupAlgorithm(SpecularReflectionPositionCorrect2 &alg, MatrixWorkspace_sptr &inWS, const double twoTheta,
                             const std::string &correctionType, const std::string &detectorName, int detectorID = 0,
                             std::optional<bool> moveFixedDetectors = std::nullopt) {
    if (!alg.isInitialized())
      alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("TwoTheta", twoTheta);
    if (!correctionType.empty())
      alg.setProperty("DetectorCorrectionType", correctionType);
    if (!detectorName.empty())
      alg.setProperty("DetectorComponentName", detectorName);
    if (detectorID > 0)
      alg.setProperty("DetectorID", detectorID);
    if (moveFixedDetectors.has_value())
      alg.setProperty("MoveFixedDetectors", moveFixedDetectors.value() ? "1" : "0");
    alg.setPropertyValue("OutputWorkspace", "test_out");
  }

  // Run the algorithm and do some basic checks. Returns the output workspace.
  static MatrixWorkspace_const_sptr runAlgorithm(SpecularReflectionPositionCorrect2 &alg) {
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    return outWS;
  }

  static void linearDetectorRotationWithFacing(MatrixWorkspace_sptr &inWS, const double twoTheta) {
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, inWS, twoTheta, "RotateAroundSample", "detector");
    alg.setProperty("DetectorFacesSample", true);
    auto outWS = runAlgorithm(alg);
    const auto &spectrumInfoOut = outWS->spectrumInfo();
    const auto nHisto = spectrumInfoOut.size();
    TS_ASSERT_DELTA(spectrumInfoOut.l2(0), spectrumInfoOut.l2(nHisto - 1), 1e-10)
    auto instrIn = inWS->getInstrument();
    auto detIn = instrIn->getComponentByName("detector");
    const auto posIn = detIn->getPos();
    const auto l2 = posIn.norm();
    auto instrOut = outWS->getInstrument();
    auto detOut = instrOut->getComponentByName("detector");
    const auto posOut = detOut->getPos();
    TS_ASSERT_DELTA(posOut.norm(), l2, 1e-10)
    const auto thetaSignDir = instrIn->getReferenceFrame()->vecThetaSign();
    const auto horizontal = thetaSignDir.X() != 0. ? true : false;
    const auto a = l2 * std::sin(twoTheta * degToRad);
    const auto x = horizontal ? a : 0.;
    const auto y = horizontal ? 0. : a;
    const auto z = l2 * std::cos(twoTheta * degToRad);
    TS_ASSERT_DELTA(posOut.X(), x, 1e-10)
    TS_ASSERT_DELTA(posOut.Y(), y, 1e-10)
    TS_ASSERT_DELTA(posOut.Z(), z, 1e-10)
  }

  static void linearDetectorRotationWithFacingAndLinePosition(MatrixWorkspace_sptr &inWS, const double twoTheta,
                                                              const double linePos, const double pixelSize) {
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, inWS, twoTheta, "RotateAroundSample", "detector");
    alg.setProperty("DetectorFacesSample", true);
    alg.setProperty("LinePosition", linePos);
    alg.setProperty("PixelSize", pixelSize);
    auto outWS = runAlgorithm(alg);
    const auto &spectrumInfoOut = outWS->spectrumInfo();
    const auto nHisto = spectrumInfoOut.size();
    TS_ASSERT_DELTA(spectrumInfoOut.l2(0), spectrumInfoOut.l2(nHisto - 1), 1e-10)
    auto instrIn = inWS->getInstrument();
    auto detIn = instrIn->getComponentByName("detector");
    const auto posIn = detIn->getPos();
    const auto l2 = posIn.norm();
    auto instrOut = outWS->getInstrument();
    auto detOut = instrOut->getComponentByName("detector");
    const auto posOut = detOut->getPos();
    TS_ASSERT_DELTA(posOut.norm(), l2, 1e-10)
    const auto lineTwoTheta = spectrumInfoOut.twoTheta(static_cast<size_t>(linePos));
    TS_ASSERT_DELTA(lineTwoTheta * radToDeg, twoTheta, 1e-10)
  }

  static double getTwoTheta(const int detID, const DetectorInfo &detInfo, const SpectrumInfo &spectrumInfo) {
    auto detIdx = detInfo.indexOf(detID);
    return spectrumInfo.twoTheta(detIdx);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionPositionCorrect2Test *createSuite() { return new SpecularReflectionPositionCorrect2Test(); }
  static void destroySuite(SpecularReflectionPositionCorrect2Test *suite) { delete suite; }

  SpecularReflectionPositionCorrect2Test() {
    FrameworkManager::Instance();

    auto load = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    load->initialize();
    load->setChild(true);
    load->setProperty("InstrumentName", "D17");
    load->setPropertyValue("OutputWorkspace", "out");
    load->execute();
    m_d17WS = load->getProperty("OutputWorkspace");
    // Crop monitors.
    auto crop = AlgorithmManager::Instance().create("CropWorkspace");
    crop->setChild(true);
    crop->setProperty("InputWorkspace", m_d17WS);
    crop->setPropertyValue("OutputWorkspace", "out");
    crop->setProperty("StartWorkspaceIndex", 0);
    crop->setProperty("EndWorkspaceIndex", 255);
    crop->execute();
    m_d17WS = crop->getProperty("OutputWorkspace");
    load->setProperty("InstrumentName", "Figaro");
    load->execute();
    m_figaroWS = load->getProperty("OutputWorkspace");
    crop->setProperty("InputWorkspace", m_figaroWS);
    crop->setPropertyValue("OutputWorkspace", "out");
    crop->execute();
    m_figaroWS = crop->getProperty("OutputWorkspace");
    load->setProperty("Filename", "INTER_Definition_2020.xml");
    load->execute();
    m_interWS = load->getProperty("OutputWorkspace");
  }

  void test_init() {
    SpecularReflectionPositionCorrect2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_detector_component_is_mandatory() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("TwoTheta", 1.4);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_detector_id_is_valid() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("DetectorID", 222222222);
    alg.setProperty("TwoTheta", 1.4);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_detector_name_is_valid() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("DetectorComponentName", "invalid-detector-name");
    alg.setProperty("TwoTheta", 1.4);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_pixel_correction_for_rectangular_detector() {
    SpecularReflectionPositionCorrect2 alg;
    const int detID = 2001;
    const double newTwoTheta = 1.4;
    setupAlgorithm(alg, m_interWS, newTwoTheta, "", "", detID, true);
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);

    auto detIn = instrIn->getDetector(detID);
    auto detOut = instrOut->getDetector(detID);
    // The pixels should have been moved
    auto posIn = detIn->getPos();
    auto posOut = detOut->getPos();
    TS_ASSERT_DIFFERS(posIn, posOut);
    // TwoTheta for the detector should have been changed
    auto thetaOut = getTwoTheta(detID, outWS->detectorInfo(), outWS->spectrumInfo());
    TS_ASSERT_DELTA(newTwoTheta, thetaOut * radToDeg, 1e-10);
  }

  void test_pixel_correction_for_rectangular_detector_ignored_by_default() {
    SpecularReflectionPositionCorrect2 alg;
    const int detID = 2001;
    setupAlgorithm(alg, m_interWS, 1.4, "", "", detID);
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);

    auto detIn = instrIn->getDetector(detID);
    auto detOut = instrOut->getDetector(detID);
    // The pixel should not have been moved
    auto posIn = detIn->getPos();
    auto posOut = detOut->getPos();
    TS_ASSERT_EQUALS(posIn, posOut);
    // TwoTheta for the detector should be unchanged
    auto thetaIn = getTwoTheta(detID, m_interWS->detectorInfo(), m_interWS->spectrumInfo());
    auto thetaOut = getTwoTheta(detID, outWS->detectorInfo(), outWS->spectrumInfo());
    TS_ASSERT_EQUALS(thetaIn, thetaOut);
  }

  void test_correct_point_detector_vertical_shift_default() {
    // Omit the DetectorCorrectionType property to check that a vertical shift
    // is done by default
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, m_interWS, 1.4, "", "point-detector");
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);
    // 'point-detector' should have been moved vertically only
    auto detIn = instrIn->getComponentByName("point-detector")->getPos();
    auto detOut = instrOut->getComponentByName("point-detector")->getPos();
    TS_ASSERT_EQUALS(detIn.X(), detOut.X());
    TS_ASSERT_EQUALS(detIn.Z(), detOut.Z());
    TS_ASSERT_DELTA(detOut.Y(), 0.06508, 1e-5);
  }

  void test_correct_point_detector_via_detid_vertical_shift_default() {
    // Omit the DetectorCorrectionType property to check that a vertical shift
    // is done by default
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, m_interWS, 1.4, "", "", 4);
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);
    // 'point-detector' should have been moved vertically only
    auto detIn = instrIn->getComponentByName("point-detector")->getPos();
    auto detOut = instrOut->getComponentByName("point-detector")->getPos();
    TS_ASSERT_EQUALS(detIn.X(), detOut.X());
    TS_ASSERT_EQUALS(detIn.Z(), detOut.Z());
    TS_ASSERT_DELTA(detOut.Y(), 0.06508, 1e-5);
  }

  void test_correct_point_detector_rotation() {
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, m_interWS, 1.4, "RotateAroundSample", "point-detector");
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);
    // 'point-detector' should have been moved  both vertically and in
    // the beam direction
    auto detIn = instrIn->getComponentByName("point-detector")->getPos();
    auto detOut = instrOut->getComponentByName("point-detector")->getPos();
    TS_ASSERT_EQUALS(detIn.X(), detOut.X());
    TS_ASSERT_DELTA(detOut.Z(), 2.66221, 1e-5);
    TS_ASSERT_DELTA(detOut.Y(), 0.06506, 1e-5);
  }

  void test_correct_point_detector_by_detid_rotation() {
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, m_interWS, 1.4, "RotateAroundSample", "", 4);
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);
    // 'point-detector' should have been moved  both vertically and in
    // the beam direction
    auto detIn = instrIn->getComponentByName("point-detector")->getPos();
    auto detOut = instrOut->getComponentByName("point-detector")->getPos();
    TS_ASSERT_EQUALS(detIn.X(), detOut.X());
    TS_ASSERT_DELTA(detOut.Z(), 2.66221, 1e-5);
    TS_ASSERT_DELTA(detOut.Y(), 0.06506, 1e-5);
  }

  void test_correct_linear_detector_vertical_shift() {
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, m_interWS, 1.4, "VerticalShift", "linear-detector");
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);
    // 'linear-detector' should have been moved vertically only
    auto detIn = instrIn->getComponentByName("linear-detector")->getPos();
    auto detOut = instrOut->getComponentByName("linear-detector")->getPos();
    TS_ASSERT_EQUALS(detIn.X(), detOut.X());
    TS_ASSERT_EQUALS(detIn.Z(), detOut.Z());
    TS_ASSERT_DELTA(detOut.Y(), 0.07730, 1e-5);
  }

  void test_correct_linear_detector_rotation() {
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, m_interWS, 1.4, "RotateAroundSample", "linear-detector");
    MatrixWorkspace_const_sptr outWS = runAlgorithm(alg);

    auto instrIn = m_interWS->getInstrument();
    auto instrOut = outWS->getInstrument();

    // Sample should not have moved
    auto sampleIn = instrIn->getSample()->getPos();
    auto sampleOut = instrOut->getSample()->getPos();
    TS_ASSERT_EQUALS(sampleIn, sampleOut);
    // 'linear-detector' should have been moved both vertically and in
    // the beam direction
    auto detIn = instrIn->getComponentByName("linear-detector")->getPos();
    auto detOut = instrOut->getComponentByName("linear-detector")->getPos();
    TS_ASSERT_EQUALS(detIn.X(), detOut.X());
    TS_ASSERT_DELTA(detOut.Z(), 3.162055, 1e-5);
    TS_ASSERT_DELTA(detOut.Y(), 0.07728, 1e-5);
  }

  void test_correct_horizontal_linear_detector_rotation_with_facing() {
    constexpr double twoTheta{1.4};
    linearDetectorRotationWithFacing(m_d17WS, twoTheta);
    linearDetectorRotationWithFacing(m_d17WS, -twoTheta);
  }

  void test_correct_vertical_linear_detector_rotation_with_facing() {
    constexpr double twoTheta{1.4};
    linearDetectorRotationWithFacing(m_figaroWS, twoTheta);
    linearDetectorRotationWithFacing(m_figaroWS, -twoTheta);
  }

  void test_correct_rotation_with_line_position() {
    constexpr double twoTheta{1.4};
    constexpr double linePos{13.};
    constexpr double pixelSize{0.0012};
    linearDetectorRotationWithFacingAndLinePosition(m_figaroWS, twoTheta, linePos, pixelSize);
  }

  void test_correct_rotation_with_line_position_when_wsindices_run_like_d17() {
    constexpr double twoTheta{1.4};
    constexpr double linePos{189.};
    constexpr double pixelSize{0.001195};
    linearDetectorRotationWithFacingAndLinePosition(m_d17WS, twoTheta, linePos, pixelSize);
  }
};
