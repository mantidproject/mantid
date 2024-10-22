// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/CreateMonteCarloWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"


using namespace Mantid::Algorithms;
using namespace Mantid::API;

using Mantid::Algorithms::CreateMonteCarloWorkspace;

class CreateMonteCarloWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateMonteCarloWorkspaceTest *createSuite() { return new CreateMonteCarloWorkspaceTest(); }
  static void destroySuite(CreateMonteCarloWorkspaceTest *suite) { delete suite; }

  void test_Init() {
    CreateMonteCarloWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("MonteCarloTest_WS");

    // Create input workspace for the algorithm
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspace(1, 10);

    CreateMonteCarloWorkspace alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    const int num_iterations = 1000;
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InstrumentWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Iterations", num_iterations));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Seed", 32));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", outWSName));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the output workspace
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // Cast to the correct type (not sure if needed)
    auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    TS_ASSERT(matrixWS);

    std::cout << "Number of histograms: " << matrixWS->getNumberHistograms() << std::endl;
    std::cout << "Size of Y data (expected " << num_iterations << "): " << matrixWS->y(0).size() << std::endl;

    TS_ASSERT_EQUALS(matrixWS->y(0).size(), num_iterations); // Check if Y data has the expected number of entries

    TS_FAIL("TODO: Remove this line once the test works as expected.");
  }

};
