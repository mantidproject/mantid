// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class PolarizationCorrectionsHelpersTest : public CxxTest::TestSuite {
public:
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testWorkspaceForFourSpinStates() { runTest({"00", "11", "10", "01"}); }

  void testWorkspaceForThreeSpinStates() { runTest({"01", "10", "00"}); }

  void testWorkspaceForTwoSpinStates() { runTest({"11", "10"}); }

  void testWorkspaceForOneSpinState() { runTest({"01"}); }

  void testSurplusWhitespace() { runTest({"01 ", "11", " 10", " 00 "}); }

private:
  WorkspaceGroup_sptr createGroupWorkspaceWithFourSpinStates(const std::vector<std::string> &spinStateOrder) {
    WorkspaceGroup_sptr grp = std::make_shared<WorkspaceGroup>();
    for (size_t i = 0; i < spinStateOrder.size(); ++i) {
      auto trimmedSpinState = trimString(spinStateOrder[i]);
      // The workspace name will be trimmed on creation
      grp->addWorkspace(createWorkspace(trimmedSpinState));
    }
    return grp;
  }

  MatrixWorkspace_sptr createWorkspace(const std::string &name) {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    createWorkspace->setProperty("DataX", std::vector<double>{0, 1});
    createWorkspace->setProperty("DataY", std::vector<double>{0, 1});
    createWorkspace->setProperty("OutputWorkspace", name);
    createWorkspace->execute();
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
    return ws;
  }

  void runTest(const std::vector<std::string> &spinStates) {
    auto spinStateOrder = boost::join(spinStates, ",");
    auto grp = createGroupWorkspaceWithFourSpinStates(spinStates);
    for (const auto &spinState : spinStates) {
      auto ws = PolarizationCorrectionsHelpers::workspaceForSpinState(grp, spinStateOrder, spinState);
      // The workspace name is not going to have any spaces in, regardless of the input. This is not
      // related to the actual workspaceForSpinState algorithm, it's just how the test is checking the
      // order
      auto trimmedSpinState = trimString(spinState);
      TS_ASSERT_EQUALS(trimmedSpinState, ws->getName());
    }
  }

  std::string trimString(const std::string &input) {
    auto trimmedInput = input;
    boost::trim(trimmedInput);
    return trimmedInput;
  }
};