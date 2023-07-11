// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidMDAlgorithms/LoadGaussCube.h"

using namespace Mantid::DataObjects;

using Mantid::MDAlgorithms::LoadGaussCube;

class LoadGaussCubeTest : public CxxTest::TestSuite {
public:
  static LoadGaussCubeTest *createSuite() { return new LoadGaussCubeTest(); }
  static void destroySuite(LoadGaussCubeTest *suite) { delete suite; }

  void test_Init() {
    LoadGaussCube alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    LoadGaussCube alg;
    // alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "gauss_cube_example.cube"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "test_md"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MDHistoWorkspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("__test_md");
    TS_ASSERT(ws);

    TS_ASSERT_EQUALS(3, ws->getNumDims());
    for (int idim = 0; idim < ws->getNumDims(); ++idim) {
      auto dim = ws->getDimension(idim);
      TS_ASSERT_EQUALS(-10.0, dim->getMaximum());
      TS_ASSERT_EQUALS(10.0, dim->getMinimum());
      TS_ASSERT_EQUALS(3, dim->getNBins());
    }

    // Check the data
    const auto signal = ws->getSignalArray();
    TS_ASSERT_DELTA(0.912648, signal[0], 0.000001); // Check the first value
  }
};
