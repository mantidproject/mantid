// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
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

    // Create test input if necessary by filling in appropriate code below.
    // Consider using MantidFrameworkTestHelpers/WorkspaceCreationHelper.h
    // MatrixWorkspace_sptr inputWS =

    CreateMonteCarloWorkspace alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Iterations", 1000));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Seed", 32));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", outWSName));

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(NumOfSim, Iterations);
    TS_ASSERT(outputWS);
    TS_FAIL("TODO: Check the results and remove this line");
  }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};
