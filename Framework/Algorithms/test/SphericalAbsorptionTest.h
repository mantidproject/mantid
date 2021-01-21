// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/SphericalAbsorption.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;

using Mantid::API::MatrixWorkspace_sptr;

class SphericalAbsorptionTest : public CxxTest::TestSuite {
public:
  void testName() {
    auto atten = Mantid::API::AlgorithmManager::Instance().create("SphericalAbsorption");
    TS_ASSERT_EQUALS(atten->name(), "SphericalAbsorption");
  }

  void testVersion() {
    auto atten = Mantid::API::AlgorithmManager::Instance().create("SphericalAbsorption");
    TS_ASSERT_EQUALS(atten->version(), 1);
  }

  void testInit() {
    auto atten = Mantid::API::AlgorithmManager::Instance().create("SphericalAbsorption");
    TS_ASSERT_THROWS_NOTHING(atten->initialize());
    TS_ASSERT(atten->isInitialized());
  }

  void testExec() {
    // register all the algorithms
    FrameworkManager::Instance();
    auto atten = Mantid::API::AlgorithmManager::Instance().create("SphericalAbsorption");
    if (!atten->isInitialized())
      atten->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_THROWS_NOTHING(atten->setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING(atten->setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(atten->setPropertyValue("SphericalSampleRadius", "0.17"));
    TS_ASSERT_THROWS_NOTHING(atten->setPropertyValue("AttenuationXSection", "1.686"));
    TS_ASSERT_THROWS_NOTHING(atten->setPropertyValue("ScatteringXSection", "1.302"));
    TS_ASSERT_THROWS_NOTHING(atten->setPropertyValue("SampleNumberDensity", "0.01"));
    TS_ASSERT_THROWS_NOTHING(atten->execute());
    TS_ASSERT(atten->isExecuted());

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_DELTA(result->readY(0).front(), 0.9942, 0.0001);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.9674, 0.0001);
    TS_ASSERT_DELTA(result->readY(0)[8], 0.9703, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }
};
