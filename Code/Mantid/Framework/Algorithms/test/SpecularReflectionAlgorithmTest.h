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

#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class SpecularReflectionAlgorithmTest
{
protected:

  MatrixWorkspace_sptr pointDetectorWS;

  MatrixWorkspace_sptr linearDetectorWS;

  void test_throws_if_SpectrumNumbersOfDetectors_less_than_zero(Mantid::API::IAlgorithm_sptr& alg)
  {
    std::vector<int> invalid(1, -1);
    TS_ASSERT_THROWS(alg->setProperty("SpectrumNumbersOfDetectors", invalid), std::invalid_argument&);
  }

public:

  SpecularReflectionAlgorithmTest()
  {
    Mantid::API::FrameworkManager::Instance();

    FrameworkManager::Instance();

    const std::string instDir = ConfigService::Instance().getInstrumentDirectory();
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
