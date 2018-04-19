#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2TEST_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SpecularReflectionPositionCorrect2.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::SpecularReflectionPositionCorrect2;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class SpecularReflectionPositionCorrect2Test : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_interWS;

  // Initialise the algorithm and set the properties
  void setupAlgorithm(SpecularReflectionPositionCorrect2 &alg,
                      const double twoTheta, const std::string &correctionType,
                      const std::string &detectorName) {
    if (!alg.isInitialized())
      alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("TwoTheta", twoTheta);
    if (!correctionType.empty())
      alg.setProperty("DetectorCorrectionType", correctionType);
    if (!detectorName.empty())
      alg.setProperty("DetectorComponentName", detectorName);
    alg.setPropertyValue("OutputWorkspace", "test_out");
  }

  // Run the algorithm and do some basic checks. Returns the output workspace.
  MatrixWorkspace_const_sptr
  runAlgorithm(SpecularReflectionPositionCorrect2 &alg) {
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outWS);
    return outWS;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionPositionCorrect2Test *createSuite() {
    return new SpecularReflectionPositionCorrect2Test();
  }
  static void destroySuite(SpecularReflectionPositionCorrect2Test *suite) {
    delete suite;
  }

  SpecularReflectionPositionCorrect2Test() {
    FrameworkManager::Instance();

    auto load = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    load->initialize();
    load->setChild(true);
    load->setProperty("InstrumentName", "INTER");
    load->setPropertyValue("OutputWorkspace", "inter");
    load->execute();
    m_interWS = load->getProperty("OutputWorkspace");
  }

  void test_init() {
    SpecularReflectionPositionCorrect2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_theta_is_mandatory() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("DetectorComponentName", "point-detector");
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error &);
  }

  void test_theta_bad_value() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("DetectorComponentName", "point-detector");
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.setProperty("TwoTheta", 0.0), std::invalid_argument &);
    TS_ASSERT_THROWS(alg.setProperty("TwoTheta", 90.0),
                     std::invalid_argument &);
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

  void test_detector_component_is_valid() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("DetectorComponentName", "invalid-detector-name");
    alg.setProperty("TwoTheta", 1.4);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_sample_component_is_valid() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("DetectorComponentName", "point-detector");
    alg.setProperty("SampleComponentName", "invalid-sample-name");
    alg.setProperty("TwoTheta", 1.4);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_correct_point_detector_vertical_shift_default() {
    // Omit the DetectorCorrectionType property to check that a vertical shift
    // is done by default
    SpecularReflectionPositionCorrect2 alg;
    setupAlgorithm(alg, 1.4, "", "point-detector");
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
    setupAlgorithm(alg, 1.4, "RotateAroundSample", "point-detector");
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
    setupAlgorithm(alg, 1.4, "VerticalShift", "linear-detector");
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
    setupAlgorithm(alg, 1.4, "RotateAroundSample", "linear-detector");
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
};

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2TEST_H_ */
