// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * SpecularReflectionAlgorithmTest.h
 *
 * Base class for common tests required by SpecularReflection type algorithms.
 *
 *  Created on: May 13, 2014
 *      Author: Owen Arnold
 */

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ConfigService.h"
#include "MantidReflectometry/SpecularReflectionAlgorithm.h"

#include <Poco/Path.h>
#include <boost/tuple/tuple.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Reflectometry;

using VerticalHorizontalOffsetType = boost::tuple<double, double>;

// This is a base class for
// SpecularReflectionPositionCorrectTest and
// SpecularReflectionCalculateThetaTest so the functions below won't be run
// directly as tests. They are confusingly named because they shouldn't really
// start with test_ if they are not intended to be run by the test suite,
// however the derived class tests do run and do make use of the base class
// functions.

class SpecularReflectionAlgorithmTest {
public:
  // This means the constructor isn't called when running other tests
  static SpecularReflectionAlgorithmTest *createSuite() {
    return new SpecularReflectionAlgorithmTest();
  }
  static void destroySuite(SpecularReflectionAlgorithmTest *suite) {
    delete suite;
  }

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
      const MatrixWorkspace_sptr &ws,
      const std::string &detectorName = "point-detector") {
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
