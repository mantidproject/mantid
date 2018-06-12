/*
 * SpecularReflectionAlgorithmTest.h
 *
 * Base class for common tests required by SpecularReflection type algorithms.
 *
 *  Created on: May 13, 2014
 *      Author: Owen Arnold
 */

#ifndef MANTID_ALGORITHMS_TEST_SPECULARREFLECTIONALGORITHMTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <boost/tuple/tuple.hpp>
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

using VerticalHorizontalOffsetType = boost::tuple<double, double>;

class SpecularReflectionAlgorithmTest {
protected:
  MatrixWorkspace_sptr pointDetectorWS;

  MatrixWorkspace_sptr linearDetectorWS;

  void test_throws_if_SpectrumNumbersOfDetectors_less_than_zero(
      Mantid::API::IAlgorithm_sptr &alg) {
    std::vector<int> invalid(1, -1);
    TS_ASSERT_THROWS(alg->setProperty("SpectrumNumbersOfDetectors", invalid),
                     std::invalid_argument &);
  }

  void test_throws_if_SpectrumNumbersOfDetectors_outside_range(
      Mantid::API::IAlgorithm_sptr &alg) {
    std::vector<int> invalid(1, static_cast<int>(1e7)); // Well outside range
    alg->setProperty("SpectrumNumbersOfDetectors",
                     invalid); // Well outside range
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument &);
  }

  void test_throws_if_DetectorComponentName_unknown(
      Mantid::API::IAlgorithm_sptr &alg) {
    alg->setProperty("DetectorComponentName",
                     "junk_value"); // This name is not known.
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument &);
  }

  VerticalHorizontalOffsetType determine_vertical_and_horizontal_offsets(
      MatrixWorkspace_sptr ws, std::string detectorName = "point-detector") {
    auto instrument = ws->getInstrument();
    const V3D pointDetector =
        instrument->getComponentByName(detectorName)->getPos();
    const V3D surfaceHolder =
        instrument->getComponentByName("some-surface-holder")->getPos();
    const auto referenceFrame = instrument->getReferenceFrame();
    const V3D sampleToDetector = pointDetector - surfaceHolder;

    const double sampleToDetectorVerticalOffset =
        sampleToDetector.scalar_prod(referenceFrame->vecPointingUp());
    const double sampleToDetectorHorizontalOffset =
        sampleToDetector.scalar_prod(referenceFrame->vecPointingAlongBeam());

    return VerticalHorizontalOffsetType(sampleToDetectorVerticalOffset,
                                        sampleToDetectorHorizontalOffset);
  }

public:
  SpecularReflectionAlgorithmTest() {
    Mantid::API::FrameworkManager::Instance();

    FrameworkManager::Instance();

    const std::string instDir =
        ConfigService::Instance().getInstrumentDirectory();
    Poco::Path path(instDir);
    path.append("INTER_Definition.xml");

    auto loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    loadAlg->initialize();
    loadAlg->setChild(true);
    loadAlg->setProperty("Filename", path.toString());
    loadAlg->setPropertyValue("OutputWorkspace", "demo");
    loadAlg->execute();
    pointDetectorWS = loadAlg->getProperty("OutputWorkspace");

    path = Poco::Path(instDir);
    path.append("POLREF_Definition.xml");
    loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
    loadAlg->initialize();
    loadAlg->setChild(true);
    loadAlg->setProperty("Filename", path.toString());
    loadAlg->setPropertyValue("OutputWorkspace", "demo");
    loadAlg->execute();
    linearDetectorWS = loadAlg->getProperty("OutputWorkspace");
  }
};

#define MANTID_ALGORITHMS_TEST_SPECULARREFLECTIONALGORITHMTEST_H_

#endif /* SPECULARREFLECTIONALGORITHMTEST_H_ */
