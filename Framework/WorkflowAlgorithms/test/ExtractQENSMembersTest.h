// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_EXTRACTQENSMEMBERSTEST_H_
#define MANTID_ALGORITHMS_EXTRACTQENSMEMBERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidDataObjects/Workspace2D.h"

#include "MantidDataHandling/Load.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidWorkflowAlgorithms/ExtractQENSMembers.h"

#include <algorithm>
#include <memory>
#include <random>

using Mantid::Algorithms::ExtractQENSMembers;

using namespace Mantid::API;
using namespace Mantid::DataObjects;

class ExtractQENSMembersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractQENSMembersTest *createSuite() {
    return new ExtractQENSMembersTest();
  }
  static void destroySuite(ExtractQENSMembersTest *suite) { delete suite; }

  ExtractQENSMembersTest() { FrameworkManager::Instance(); }

  void test_basic_extraction() {
    std::string runName = "irs26173";
    std::string runSample = "graphite002";
    std::string fileName = runName + "_" + runSample;

    std::string outputName = "Extracted";
    std::vector<std::string> members = {"MemberA", "MemberB", "MemberC"};

    auto inputWS = loadWorkspace(fileName + "_red.nxs");
    const auto numSpectra = inputWS->getNumberHistograms();
    const auto &dataX = inputWS->dataX(0);
    auto resultGroup = createResultGroup(members, dataX, numSpectra);

    WorkspaceGroup_sptr membersWorkspace =
        extractMembers(inputWS, resultGroup, outputName);
    membersWorkspace->sortByName();

    checkMembersOutput(membersWorkspace, members, outputName, numSpectra,
                       dataX);

    AnalysisDataService::Instance().clear();
  }

  void test_extraction_rename_convolved() {
    std::string runName = "irs26173";
    std::string runSample = "graphite002";
    std::string fileName = runName + "_" + runSample;

    std::string outputName = "Extracted";
    std::vector<std::string> original = {"MemberA", "MemberB", "MemberC",
                                         "MemberD"};
    std::vector<std::string> members = {"MemberA", "Convolution", "Convolution",
                                        "MemberD"};
    std::vector<std::string> convolved = {"MemberB", "MemberC"};

    auto inputWS = loadWorkspace(fileName + "_red.nxs");
    const auto numSpectra = inputWS->getNumberHistograms();
    const auto &dataX = inputWS->dataX(0);
    auto resultGroup = createResultGroup(members, dataX, numSpectra);

    WorkspaceGroup_sptr membersWorkspace =
        extractMembers(inputWS, resultGroup, convolved, outputName);
    membersWorkspace->sortByName();

    checkMembersOutput(membersWorkspace, original, outputName, numSpectra,
                       dataX);

    AnalysisDataService::Instance().clear();
  }

private:
  void checkMembersOutput(WorkspaceGroup_sptr membersWorkspace,
                          const std::vector<std::string> &members,
                          const std::string &outputName, size_t numSpectra,
                          const std::vector<double> &dataX) const {
    for (auto i = 0u; i < members.size(); ++i) {
      auto memberWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
          membersWorkspace->getItem(i));
      const auto &memberName = memberWorkspace->getName();
      const auto expectedName = outputName + "_" + members[i];
      const auto numMemberSpectra = memberWorkspace->getNumberHistograms();
      const auto &memberDataX = memberWorkspace->dataX(0);

      TS_ASSERT_EQUALS(dataX, memberDataX);
      TS_ASSERT_EQUALS(numSpectra, numMemberSpectra);
      TS_ASSERT_EQUALS(memberName, expectedName);
    }
  }

  WorkspaceGroup_sptr extractMembers(MatrixWorkspace_sptr inputWs,
                                     WorkspaceGroup_sptr resultGroupWs,
                                     const std::string &outputWsName) const {
    auto extractAlgorithm =
        extractMembersAlgorithm(inputWs, resultGroupWs, outputWsName);
    extractAlgorithm->execute();
    return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        outputWsName);
  }

  WorkspaceGroup_sptr
  extractMembers(MatrixWorkspace_sptr inputWs,
                 WorkspaceGroup_sptr resultGroupWs,
                 const std::vector<std::string> &convolvedMembers,
                 const std::string &outputWsName) const {
    auto extractAlgorithm = extractMembersAlgorithm(
        inputWs, resultGroupWs, convolvedMembers, outputWsName);
    extractAlgorithm->execute();
    return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        outputWsName);
  }

  IAlgorithm_sptr
  extractMembersAlgorithm(MatrixWorkspace_sptr inputWs,
                          WorkspaceGroup_sptr resultGroupWs,
                          const std::string &outputWsName) const {
    auto extractMembersAlg =
        AlgorithmManager::Instance().create("ExtractQENSMembers");
    extractMembersAlg->setProperty("InputWorkspace", inputWs);
    extractMembersAlg->setProperty("ResultWorkspace", resultGroupWs);
    extractMembersAlg->setProperty("OutputWorkspace", outputWsName);
    return extractMembersAlg;
  }

  IAlgorithm_sptr
  extractMembersAlgorithm(MatrixWorkspace_sptr inputWs,
                          WorkspaceGroup_sptr resultGroupWs,
                          const std::vector<std::string> &convolvedMembers,
                          const std::string &outputWsName) const {
    auto extractMembersAlg =
        AlgorithmManager::Instance().create("ExtractQENSMembers");
    extractMembersAlg->setProperty("InputWorkspace", inputWs);
    extractMembersAlg->setProperty("ResultWorkspace", resultGroupWs);
    extractMembersAlg->setProperty("OutputWorkspace", outputWsName);
    extractMembersAlg->setProperty("RenameConvolvedMembers", true);
    extractMembersAlg->setProperty("ConvolvedMembers", convolvedMembers);
    return extractMembersAlg;
  }

  WorkspaceGroup_sptr createResultGroup(const std::vector<std::string> &members,
                                        const std::vector<double> &dataX,
                                        size_t numSpectra) const {
    std::vector<std::string> resultWorkspaces;
    resultWorkspaces.reserve(numSpectra);

    for (auto i = 0u; i < numSpectra; ++i) {
      const auto name = "Result_" + std::to_string(i);
      AnalysisDataService::Instance().addOrReplace(
          name, createResultWorkspace(members, dataX));
      resultWorkspaces.emplace_back(name);
    }
    return groupWorkspaces(resultWorkspaces);
  }

  MatrixWorkspace_sptr
  createResultWorkspace(const std::vector<std::string> &members,
                        const std::vector<double> &dataX) const {
    MatrixWorkspace_sptr resultWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceNonUniformlyBinned(
            static_cast<int>(members.size()), static_cast<int>(dataX.size()),
            dataX.data());

    auto axis = std::make_unique<TextAxis>(members.size());
    for (auto i = 0u; i < members.size(); ++i) {
      auto memberWS = createGenericWorkspace(
          dataX, randomDataVector<double>(dataX.size() - 1, 0.0, 10.0));
      memberWS->getAxis(0)->setUnit(
          resultWorkspace->getAxis(0)->unit()->unitID());
      resultWorkspace = appendSpectra(resultWorkspace, memberWS);
      axis->setLabel(i, members[i]);
    }
    resultWorkspace->replaceAxis(1, axis.release());
    return resultWorkspace;
  }

  MatrixWorkspace_sptr
  createGenericWorkspace(const std::vector<double> &dataX,
                         const std::vector<double> &dataY) const {
    auto createWorkspace = createWorkspaceAlgorithm(dataX, dataY);
    createWorkspace->execute();
    return createWorkspace->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr appendSpectra(MatrixWorkspace_sptr workspace,
                                     MatrixWorkspace_sptr spectraWS) const {
    auto appendAlgorithm = appendSpectraAlgorithm(workspace, spectraWS);
    appendAlgorithm->execute();
    return appendAlgorithm->getProperty("OutputWorkspace");
  }

  WorkspaceGroup_sptr
  groupWorkspaces(const std::vector<std::string> &workspaces) const {
    auto groupAlgorithm = groupWorkspacesAlgorithm(workspaces);
    groupAlgorithm->execute();
    return groupAlgorithm->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr loadWorkspace(const std::string &fileName) {
    auto loadAlgorithm = loadWorkspaceAlgorithm(fileName);
    loadAlgorithm->execute();
    Workspace_sptr workspace = loadAlgorithm->getProperty("OutputWorkspace");
    return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
  }

  IAlgorithm_sptr loadWorkspaceAlgorithm(const std::string &fileName) {
    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->setChild(true);
    loadAlg->setProperty("Filename", fileName);
    loadAlg->setProperty("OutputWorkspace", "__temp");
    return loadAlg;
  }

  IAlgorithm_sptr
  groupWorkspacesAlgorithm(const std::vector<std::string> &workspaces) const {
    auto groupWorkspaces =
        AlgorithmManager::Instance().create("GroupWorkspaces");
    groupWorkspaces->setChild(true);
    groupWorkspaces->setProperty("InputWorkspaces", workspaces);
    groupWorkspaces->setProperty("OutputWorkspace", "__grouped");
    return groupWorkspaces;
  }

  IAlgorithm_sptr
  createWorkspaceAlgorithm(const std::vector<double> &dataX,
                           const std::vector<double> &dataY) const {
    auto createWorkspace =
        AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->setChild(true);
    createWorkspace->setProperty("DataX", dataX);
    createWorkspace->setProperty("DataY", dataY);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setProperty("OutputWorkspace", "__workspace");
    return createWorkspace;
  }

  IAlgorithm_sptr appendSpectraAlgorithm(MatrixWorkspace_sptr workspace,
                                         MatrixWorkspace_sptr spectraWS) const {
    auto appendAlgorithm = AlgorithmManager::Instance().create("AppendSpectra");
    appendAlgorithm->setChild(true);
    appendAlgorithm->setProperty("InputWorkspace1", workspace);
    appendAlgorithm->setProperty("InputWorkspace2", spectraWS);
    appendAlgorithm->setProperty("OutputWorkspace", "__appended");
    return appendAlgorithm;
  }

  template <typename T>
  std::vector<T>
  randomDataVector(size_t size, T const &min = std::numeric_limits<T>::min(),
                   T const &max = std::numeric_limits<T>::max()) const {
    static std::random_device rd;
    static std::uniform_real_distribution<T> distribution(min, max);
    static std::mt19937 generator(rd());

    std::vector<T> data(size);
    std::generate(data.begin(), data.end(),
                  []() { return distribution(generator); });
    return data;
  }
};

#endif /* MANTID_ALGORITHMS_EXTRACTQENSMEMBERSTEST_H_ */
