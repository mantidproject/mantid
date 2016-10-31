#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2TEST_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2TEST_H_

#include <cxxtest/TestSuite.h>
#include "SpecularReflectionAlgorithmTest.h"

#include "MantidAlgorithms/SpecularReflectionPositionCorrect2.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <cmath>

using Mantid::Algorithms::SpecularReflectionPositionCorrect2;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

// clang-format off
class SpecularReflectionPositionCorrect2Test: public CxxTest::TestSuite,
    public SpecularReflectionAlgorithmTest
      // clang-format on
      {
private:
	MatrixWorkspace_sptr m_interWS;
	MatrixWorkspace_sptr m_offspecWS;
	MatrixWorkspace_sptr m_polrefWS;
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

    auto loadINTER = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    loadINTER->initialize();
    loadINTER->setChild(true);
    loadINTER->setProperty("InstrumentName", "INTER");
    loadINTER->setPropertyValue("OutputWorkspace", "inter");
    loadINTER->execute();
    m_interWS = loadINTER->getProperty("OutputWorkspace");

    auto loadOFFSPEC = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    loadOFFSPEC->initialize();
    loadOFFSPEC->setChild(true);
    loadOFFSPEC->setProperty("InstrumentName", "OFFSPEC");
    loadOFFSPEC->setPropertyValue("OutputWorkspace", "offspec");
    loadOFFSPEC->execute();
    m_offspecWS = loadOFFSPEC->getProperty("OutputWorkspace");

    auto loadPOLREF = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    loadPOLREF->initialize();
    loadPOLREF->setChild(true);
    loadPOLREF->setProperty("InstrumentName", "POLREF");
    loadPOLREF->setPropertyValue("OutputWorkspace", "polref");
    loadPOLREF->execute();
    m_polrefWS = loadPOLREF->getProperty("OutputWorkspace");
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
    TS_ASSERT_THROWS(alg.setProperty("TwoThetaIn", 0.0),
                     std::invalid_argument &);
    TS_ASSERT_THROWS(alg.setProperty("TwoThetaIn", 90.0),
                     std::invalid_argument &);
  }

  void test_detector_component_is_mandatory() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("TwoThetaIn", 1.4);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_correct_point_detector() {
    SpecularReflectionPositionCorrect2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", m_interWS);
    alg.setProperty("TwoThetaIn", 1.4);
    alg.setProperty("DetectorComponentName", "point-detector");
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    
    TS_ASSERT(outWS);
  }

  void test_correct_line_detector() {
  }

  void test_strict_spectrum_checking() {
  }
};

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECT2TEST_H_ */
