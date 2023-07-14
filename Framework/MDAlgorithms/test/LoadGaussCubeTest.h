// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Units", "rlu,rlu,rlu"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Frames", "HKL,HKL,HKL"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Names", "[H,0,0],[0,K,0],[0,0,L]"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MDHistoWorkspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("test_md");
    TS_ASSERT(ws);

    TS_ASSERT_EQUALS(3, ws->getNumDims());
    for (size_t idim = 0; idim < ws->getNumDims(); ++idim) {
      auto dim = ws->getDimension(idim);
      TS_ASSERT_DELTA(-10.0, dim->getMinimum(), 1e-6);
      TS_ASSERT_DELTA(10.0, dim->getMaximum(), 1e-6);
      TS_ASSERT_DELTA(3, dim->getNBins(), 1e-6);
    }

    // Check the data
    const auto signal = ws->getSignalArray();
    TS_ASSERT_DELTA(0.912648, signal[0], 1e-6);
    TS_ASSERT_DELTA(0.512429, signal[5], 1e-6);
    TS_ASSERT_DELTA(0.954200, signal[26], 1e-6);
  }
};
