// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/SpectralMomentMD.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using Mantid::MDAlgorithms::SpectralMomentMD;

class SpectralMomentMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectralMomentMDTest *createSuite() { return new SpectralMomentMDTest(); }
  static void destroySuite(SpectralMomentMDTest *suite) { delete suite; }

  void test_Init() {
    SpectralMomentMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Create input workspace
    for (int power = -3; power <= 3; power++) {
      SpectralMomentMD alg;
      // Don't put output in ADS by default
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", mMDEventWSName));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", mOutputWorkspaceName));
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Moment", power));
      TS_ASSERT_THROWS_NOTHING(alg.execute(););
      TS_ASSERT(alg.isExecuted());

      // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
      // be the type using in declareProperty for the "OutputWorkspace" type.
      // We can't use auto as it's an implicit conversion.
      Mantid::API::IMDEventWorkspace_sptr outputWS;
      TS_ASSERT_THROWS_NOTHING(outputWS = std::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(
                                   Mantid::API::AnalysisDataService::Instance().retrieve(mOutputWorkspaceName)));
      TS_ASSERT(outputWS);
      auto it = outputWS->createIterator(nullptr);
      TS_ASSERT_EQUALS(it->getNumEvents(), 21);
      for (int index = 0; index < 21; index++) {
        TS_ASSERT_DELTA(it->getInnerSignal(index), std::pow(static_cast<double>(index) - 10., power), 1e-5);
        TS_ASSERT_DELTA(it->getInnerError(index), std::abs(std::pow(static_cast<double>(index) - 10., power)), 1e-5);
      }
    }
  }

  void setUp() override {
    mMDEventWSName = "SpectralMomentRawMDEvent";
    mMDEventWSNameWrong = "SpectralMomentRawMDEventWrong";
    mOutputWorkspaceName = "SpectralMomentOutput";

    // Create input workspaces
    auto create_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateMDWorkspace");
    create_alg->initialize();
    create_alg->setPropertyValue("EventType", "MDEvent");
    create_alg->setProperty("Names", "DeltaE");
    create_alg->setProperty("Extents", "-10,11");
    create_alg->setProperty("Units", "meV");
    create_alg->setProperty("SplitInto", "1");
    create_alg->setProperty("MaxRecursionDepth", "1");
    create_alg->setPropertyValue("OutputWorkspace", mMDEventWSName);
    create_alg->execute();

    create_alg->initialize();
    create_alg->setPropertyValue("EventType", "MDEvent");
    create_alg->setProperty("Names", "WrongUnits");
    create_alg->setProperty("Extents", "-10,11");
    create_alg->setPropertyValue("OutputWorkspace", mMDEventWSNameWrong);
    create_alg->execute();

    auto fake_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("FakeMDEventData");
    fake_alg->initialize();
    fake_alg->setPropertyValue("InputWorkspace", mMDEventWSName);
    fake_alg->setPropertyValue("UniformParams", "-21");
    fake_alg->execute();
  }

  void tearDown() override {
    // clean up
    Mantid::API::AnalysisDataService::Instance().remove(mMDEventWSName);
    Mantid::API::AnalysisDataService::Instance().remove(mMDEventWSNameWrong);
    Mantid::API::AnalysisDataService::Instance().remove(mOutputWorkspaceName);
  }

private:
  std::string mMDEventWSName;
  std::string mMDEventWSNameWrong;
  std::string mOutputWorkspaceName;
};
