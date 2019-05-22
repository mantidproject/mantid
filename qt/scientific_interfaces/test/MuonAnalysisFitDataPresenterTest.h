// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTERTEST_H_

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../Muon/MuonAnalysisDataLoader.h"
#include "../Muon/MuonAnalysisFitDataPresenter.h"
#include "../Muon/MuonAnalysisHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IMuonFitDataModel.h"
#include "MantidQtWidgets/Common/IMuonFitDataSelector.h"
#include "MantidQtWidgets/Common/IWorkspaceFitControl.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::ITableWorkspace;
using Mantid::API::TableRow;
using Mantid::API::Workspace;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceGroup;
using MantidQt::CustomInterfaces::MuonAnalysisDataLoader;
using MantidQt::CustomInterfaces::MuonAnalysisFitDataPresenter;
using MantidQt::CustomInterfaces::Muon::DeadTimesType;
using MantidQt::MantidWidgets::IMuonFitDataModel;
using MantidQt::MantidWidgets::IMuonFitDataSelector;
using MantidQt::MantidWidgets::IWorkspaceFitControl;
using namespace testing;

/// This is necessary for using Google Mock with boost::optional
/// (the RHEL6 build fails if this is not present)
namespace boost {
template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait> &
operator<<(std::basic_ostream<CharType, CharTrait> &out,
           optional<QString> const &maybe) {
  if (maybe)
    out << maybe->toStdString();
  return out;
}
} // namespace boost
/// Mock data selector widget
class MockDataSelector : public IMuonFitDataSelector {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE

  MOCK_CONST_METHOD0(getFilenames, QStringList());
  MOCK_CONST_METHOD0(getStartTime, double());
  MOCK_CONST_METHOD0(getEndTime, double());
  MOCK_METHOD1(setPeriodsSelected, void(const QStringList &));
  MOCK_CONST_METHOD0(getPeriodSelections, QStringList());
  MOCK_METHOD3(setWorkspaceDetails, void(const QString &, const QString &,
                                         const boost::optional<QString> &));
  MOCK_CONST_METHOD0(getChosenGroups, QStringList());
  MOCK_METHOD1(setGroupsSelected, void(const QStringList &));
  MOCK_METHOD1(setStartTime, void(double));
  MOCK_METHOD1(setEndTime, void(double));
  MOCK_METHOD1(setStartTimeQuietly, void(double));
  MOCK_METHOD1(setEndTimeQuietly, void(double));
  MOCK_CONST_METHOD0(getFitType, IMuonFitDataSelector::FitType());
  MOCK_CONST_METHOD0(getInstrumentName, QString());
  MOCK_CONST_METHOD0(getRuns, QString());
  MOCK_CONST_METHOD0(getSimultaneousFitLabel, QString());
  MOCK_METHOD1(setSimultaneousFitLabel, void(const QString &));
  MOCK_CONST_METHOD0(getDatasetIndex, int());
  MOCK_METHOD1(setDatasetNames, void(const QStringList &));
  MOCK_CONST_METHOD0(getDatasetName, QString());
  MOCK_METHOD0(askUserWhetherToOverwrite, bool());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

/// Mock fit property browser
class MockFitBrowser : public IWorkspaceFitControl, public IMuonFitDataModel {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(setWorkspaceName, void(const QString &));
  MOCK_METHOD1(setStartX, void(double));
  MOCK_METHOD1(setEndX, void(double));
  MOCK_METHOD1(setWorkspaceIndex, void(int));
  MOCK_METHOD1(allowSequentialFits, void(bool));
  MOCK_METHOD1(setWorkspaceNames, void(const QStringList &));
  MOCK_METHOD1(workspacesToFitChanged, void(int));
  MOCK_METHOD1(setSimultaneousLabel, void(const std::string &));
  MOCK_METHOD1(userChangedDataset, void(int));
  MOCK_CONST_METHOD0(rawData, bool());
  MOCK_METHOD1(continueAfterChecks, void(bool));
  MOCK_METHOD1(setNumPeriods, void(size_t));
  MOCK_METHOD1(setAvailableGroups, void(const QStringList &));
  MOCK_METHOD1(setChosenGroup, void(const QString &));
  void preFitChecksRequested(bool sequential) override {
    UNUSED_ARG(sequential);
  }
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

class MuonAnalysisFitDataPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisFitDataPresenterTest *createSuite() {
    return new MuonAnalysisFitDataPresenterTest();
  }
  static void destroySuite(MuonAnalysisFitDataPresenterTest *suite) {
    delete suite;
  }

  /// Constructor
  MuonAnalysisFitDataPresenterTest()
      : m_dataLoader(DeadTimesType::None,
                     QStringList{"MUSR", "EMU", "HIFI", "ARGUS", "CHRONUS"}) {
    Mantid::API::FrameworkManager::Instance();
  }

  /// Run before each test to create mock objects
  void setUp() override {
    Mantid::API::Grouping grouping;
    grouping.groupNames = {"fwd", "bwd"};
    grouping.pairNames = {"long"};
    grouping.groups = {"1-32", "33-64"};
    grouping.pairs.emplace_back(0, 1);
    grouping.pairAlphas = {1.0};
    m_dataSelector = new NiceMock<MockDataSelector>();
    m_fitBrowser = new NiceMock<MockFitBrowser>();
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return(QString("Label")));
    m_presenter = new MuonAnalysisFitDataPresenter(
        m_fitBrowser, m_dataSelector, m_dataLoader, grouping,
        MantidQt::CustomInterfaces::Muon::PlotType::Asymmetry);
  }

  /// Run after each test to check expectations and remove mocks
  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_dataSelector));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_fitBrowser));
    delete m_dataSelector;
    delete m_fitBrowser;
    delete m_presenter;
    AnalysisDataService::Instance().clear();
  }

  void test_handleDataPropertiesChanged() {
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.3));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(9.9));
    EXPECT_CALL(*m_fitBrowser, setWorkspaceIndex(0)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setStartX(0.3)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setEndX(9.9)).Times(1);
    m_presenter->handleDataPropertiesChanged();
  }

  void test_handleSelectedDataChanged_Simultaneous() {
    doTest_handleSelectedDataChanged(
        IMuonFitDataSelector::FitType::Simultaneous);
  }

  void test_handleSelectedDataChanged_CoAdd() {
    doTest_handleSelectedDataChanged(IMuonFitDataSelector::FitType::CoAdd);
  }

  void test_handleSelectedDataChanged_shouldUpdateLabel() {
    setupForDataChange();
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return(QString("15000-3"))); // the previous run numbers
    EXPECT_CALL(*m_dataSelector, setSimultaneousFitLabel(QString("15189-91")))
        .Times(1);
    EXPECT_CALL(*m_fitBrowser, setSimultaneousLabel("15189-91")).Times(1);
    m_presenter->handleSelectedDataChanged(false);
  }

  void
  test_handleSelectedDataChanged_labelSetToNonDefaultValue_shouldNotUpdateLabel() {
    setupForDataChange();
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return(QString("UserSelectedFitLabel")));
    EXPECT_CALL(*m_dataSelector, setSimultaneousFitLabel(_)).Times(0);
    EXPECT_CALL(*m_fitBrowser, setSimultaneousLabel(_)).Times(0);
    m_presenter->handleSelectedDataChanged(false);
  }

  void test_handleXRangeChangedGraphically() {
    EXPECT_CALL(*m_dataSelector, setStartTimeQuietly(0.4)).Times(1);
    EXPECT_CALL(*m_dataSelector, setEndTimeQuietly(9.4)).Times(1);
    m_presenter->handleXRangeChangedGraphically(0.4, 9.4);
  }

  void test_setAssignedFirstRun_singleWorkspace() {
    setupGroupPeriodSelections();
    const QString wsName("MUSR00015189; Pair; long; Asym; 1; #1");
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00015189"), QString("MUSR"),
                                    Eq(boost::optional<QString>{})))
        .Times(1);
    m_presenter->setAssignedFirstRun(wsName, boost::none);
  }

  void test_setAssignedFirstRun_contiguousRange() {
    setupGroupPeriodSelections();
    const QString wsName("MUSR00015189-91; Pair; long; Asym; 1; #1");
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00015189-91"), QString("MUSR"),
                                    Eq(boost::optional<QString>{})))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setPeriodsSelected(QStringList({"1"})))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setGroupsSelected(QStringList({"long"})))
        .Times(1);
    localSetAssignedFirstRun(wsName, boost::none);
  }

  void test_setAssignedFirstRun_nonContiguousRange() {
    setupGroupPeriodSelections();
    const QString wsName("MUSR00015189-91, 15193; Pair; long; Asym; 1; #1");
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00015189-91, 15193"),
                                    QString("MUSR"),
                                    Eq(boost::optional<QString>{})))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setGroupsSelected(QStringList({"long"})))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setPeriodsSelected(QStringList({"1"})))
        .Times(1);
    // m_presenter->setAssignedFirstRun(wsName, boost::none);
    localSetAssignedFirstRun(wsName, boost::none);
  }

  void test_setAssignedFirstRun_alreadySet() {
    setupGroupPeriodSelections();
    const QString wsName("MUSR00015189; Pair; long; Asym; 1; #1");
    m_presenter->setAssignedFirstRun(wsName, boost::none);
    EXPECT_CALL(*m_dataSelector, setWorkspaceDetails(_, _, _)).Times(0);
    EXPECT_CALL(*m_fitBrowser, allowSequentialFits(_)).Times(0);
    EXPECT_CALL(*m_dataSelector, setGroupsSelected(QStringList({"long"})))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setPeriodsSelected(QStringList({"1"})))
        .Times(1);
    // m_presenter->setAssignedFirstRun(wsName, boost::none);
    localSetAssignedFirstRun(wsName, boost::none);
  }

  void test_setAssignedFirstRun_loadCurrentRun() {
    setupGroupPeriodSelections();
    const boost::optional<QString> currentRunPath{
        R"(\\musr\data\MUSRauto_A.tmp)"};
    const QString wsName("MUSR00061335; Pair; long; Asym; 1; #1");
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00061335"), QString("MUSR"),
                                    Eq(currentRunPath)))
        .Times(1);
    // m_presenter->setAssignedFirstRun(wsName, currentRunPath);
    localSetAssignedFirstRun(wsName, currentRunPath);
  }

  void test_getAssignedFirstRun() {
    setupGroupPeriodSelections();
    const QString wsName("MUSR00015189; Pair; long; Asym; 1; #1");
    // m_presenter->setAssignedFirstRun(wsName, boost::none);
    localSetAssignedFirstRun(wsName, boost::none);
    TS_ASSERT_EQUALS(wsName, m_presenter->getAssignedFirstRun());
  }

  void test_handleSimultaneousFitLabelChanged() {
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return("UserSelectedFitLabel"));
    EXPECT_CALL(*m_fitBrowser,
                setSimultaneousLabel(std::string("UserSelectedFitLabel")))
        .Times(1);
    m_presenter->handleSimultaneousFitLabelChanged();
  }

  void test_handleFitFinished_nonSequential() {
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return("UserSelectedFitLabel"));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(IMuonFitDataSelector::FitType::Single));
    EXPECT_CALL(*m_dataSelector, getChosenGroups())
        .Times(1)
        .WillOnce(Return(QStringList({"fwd"})));
    EXPECT_CALL(*m_dataSelector, getPeriodSelections())
        .Times(1)
        .WillOnce(Return(QStringList({"1"})));
    createFittedWorkspacesGroup("UserSelectedFitLabel",
                                {"MUSR00015189; Group; fwd; Asym; 1; #1"});
    const auto workspacesBefore =
        AnalysisDataService::Instance().getObjectNames();
    m_presenter->handleFitFinished();
    const auto workspacesAfter =
        AnalysisDataService::Instance().getObjectNames();
    // assert nothing has happened
    TS_ASSERT_EQUALS(workspacesBefore, workspacesAfter);
  }

  void test_handleFitFinished_oneRunMultiplePeriods() {
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return("UserSelectedFitLabel"));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(IMuonFitDataSelector::FitType::Single));
    EXPECT_CALL(*m_dataSelector, getChosenGroups())
        .Times(1)
        .WillOnce(Return(QStringList({"fwd"})));
    EXPECT_CALL(*m_dataSelector, getPeriodSelections())
        .Times(1)
        .WillOnce(Return(QStringList({"1", "2"})));
    createFittedWorkspacesGroup("UserSelectedFitLabel",
                                {"MUSR00015189; Group; fwd; Asym; 1; #1",
                                 "MUSR00015189; Group; fwd; Asym; 2; #1"});
    const auto workspacesBefore =
        AnalysisDataService::Instance().getObjectNames();
    m_presenter->handleFitFinished();
    const auto workspacesAfter =
        AnalysisDataService::Instance().getObjectNames();
    // assert something has happened
    TS_ASSERT_DIFFERS(workspacesBefore, workspacesAfter);
  }

  void test_handleFitFinished_oneRunMultipleGroups() {
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return("UserSelectedFitLabel"));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(IMuonFitDataSelector::FitType::CoAdd));
    EXPECT_CALL(*m_dataSelector, getChosenGroups())
        .Times(1)
        .WillOnce(Return(QStringList({"fwd", "bwd"})));
    ON_CALL(*m_dataSelector, getPeriodSelections())
        .WillByDefault(Return(QStringList({"1"})));
    createFittedWorkspacesGroup("UserSelectedFitLabel",
                                {"MUSR00015189-90; Group; fwd; Asym; 1; #1",
                                 "MUSR00015189-90; Group; bwd; Asym; 1; #1"});
    const auto workspacesBefore =
        AnalysisDataService::Instance().getObjectNames();
    m_presenter->handleFitFinished();
    const auto workspacesAfter =
        AnalysisDataService::Instance().getObjectNames();
    // assert something has happened
    TS_ASSERT_DIFFERS(workspacesBefore, workspacesAfter);
  }

  void test_handleFitFinished_simultaneous() {
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return("UserSelectedFitLabel"));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(IMuonFitDataSelector::FitType::Simultaneous));
    ON_CALL(*m_dataSelector, getChosenGroups())
        .WillByDefault(Return(QStringList({"long"})));
    ON_CALL(*m_dataSelector, getPeriodSelections())
        .WillByDefault(Return(QStringList({"1"})));
    const std::string label = "UserSelectedFitLabel";
    const std::vector<std::string> inputNames{
        "MUSR00015189; Pair; long; Asym; 1; #1",
        "MUSR00015190; Pair; long; Asym; 1; #1"};
    createFittedWorkspacesGroup(label, inputNames);
    m_presenter->handleFitFinished();
    checkFittedWorkspacesHandledCorrectly(label, inputNames);
  }

  void test_handleFitFinished_cannotFindWorkspaces_doesNotThrow() {
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return("UniqueLabelThatIsNotInTheADS"));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(IMuonFitDataSelector::FitType::Simultaneous));
    ON_CALL(*m_dataSelector, getChosenGroups())
        .WillByDefault(Return(QStringList({"long"})));
    ON_CALL(*m_dataSelector, getPeriodSelections())
        .WillByDefault(Return(QStringList({"1"})));
    TS_ASSERT_THROWS_NOTHING(m_presenter->handleFitFinished());
  }

  void test_handleDatasetIndexChanged() {
    const int index = 2;
    EXPECT_CALL(*m_fitBrowser, userChangedDataset(index)).Times(1);
    m_presenter->handleDatasetIndexChanged(index);
  }

  void test_generateWorkspaceNames_CoAdd() {
    doTest_generateWorkspaceNames(IMuonFitDataSelector::FitType::CoAdd, false,
                                  {"MUSR00015189-91; Pair; long; Asym; 1; #1"});
  }

  void test_generateWorkspaceNames_CoAdd_Raw() {
    doTest_generateWorkspaceNames(
        IMuonFitDataSelector::FitType::CoAdd, true,
        {"MUSR00015189-91; Pair; long; Asym; 1; #1_Raw"});
  }

  void test_generateWorkspaceNames_Simultaneous() {
    doTest_generateWorkspaceNames(IMuonFitDataSelector::FitType::Simultaneous,
                                  false,
                                  {"MUSR00015189; Pair; long; Asym; 1; #1",
                                   "MUSR00015190; Pair; long; Asym; 1; #1",
                                   "MUSR00015191; Pair; long; Asym; 1; #1"});
  }

  void test_generateWorkspaceNames_Simultaneous_Raw() {
    doTest_generateWorkspaceNames(
        IMuonFitDataSelector::FitType::Simultaneous, true,
        {"MUSR00015189; Pair; long; Asym; 1; #1_Raw",
         "MUSR00015190; Pair; long; Asym; 1; #1_Raw",
         "MUSR00015191; Pair; long; Asym; 1; #1_Raw"});
  }

  void test_generateWorkspaceNames_noInstrument() {
    const auto &names =
        m_presenter->generateWorkspaceNames("", "15189-91", false);
    TS_ASSERT(names.empty());
  }

  void test_generateWorkspaceNames_noRuns() {
    const auto &names = m_presenter->generateWorkspaceNames("MUSR", "", false);
    TS_ASSERT(names.empty());
  }

  void test_createWorkspacesToFit_AlreadyExists() {
    // Put workspace into ADS under this name
    auto &ads = AnalysisDataService::Instance();
    const std::vector<std::string> names{
        "MUSR00015189; Pair; long; Asym; 1; #1"};
    const auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    ads.add(names[0], ws);
    m_presenter->createWorkspacesToFit(names);
    // Ensure workspace has not been replaced in ADS
    const auto retrievedWS =
        ads.retrieveWS<Mantid::API::MatrixWorkspace>(names[0]);
    TS_ASSERT(retrievedWS);
    TS_ASSERT(Mantid::API::equals(retrievedWS, ws));
  }

  void test_createWorkspacesToFit() {
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.1));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(9.9));
    auto &ads = AnalysisDataService::Instance();
    const std::vector<std::string> names{
        "MUSR00015189; Pair; long; Asym; 1; #1",
        "MUSR00015189; Group; fwd; Asym; 1; #1"};
    m_presenter->createWorkspacesToFit(names);
    // Make sure workspaces have been created and grouped together
    const auto group = ads.retrieveWS<WorkspaceGroup>("MUSR00015189");
    TS_ASSERT(group);
    for (const auto &name : names) {
      TS_ASSERT(group->contains(name));
    }
  }

  void test_createWorkspacesToFit_RawData() {
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.1));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(9.9));
    auto &ads = AnalysisDataService::Instance();
    const std::vector<std::string> names{
        "MUSR00015189; Pair; long; Asym; 1; #1_Raw",
        "MUSR00015189; Group; fwd; Asym; 1; #1_Raw"};
    m_presenter->createWorkspacesToFit(names);
    // Make sure workspaces have been created and grouped together
    const auto group = ads.retrieveWS<WorkspaceGroup>("MUSR00015189");
    TS_ASSERT(group);
    for (const auto &name : names) {
      TS_ASSERT(group->contains(name));
    }
  }

  void test_handleFittedWorkspaces_defaultGroupName() {
    const std::string label = "UserSelectedFitLabel";
    const std::vector<std::string> inputNames{
        "MUSR00015189; Pair; long; Asym; 1; #1",
        "MUSR00015190; Pair; long; Asym; 1; #1"};
    createFittedWorkspacesGroup(label, inputNames);
    const std::string baseName = "MuonSimulFit_" + label;
    m_presenter->handleFittedWorkspaces(baseName);
    // extracted = false as we have not called extractFittedWorkspaces
    checkFittedWorkspacesHandledCorrectly(label, inputNames, false);
  }

  void test_handleFittedWorkspaces_otherGroupName() {
    const std::string label = "UserSelectedFitLabel";
    const std::vector<std::string> inputNames{
        "MUSR00015189; Pair; long; Asym; 1; #1",
        "MUSR00015189; Group; fwd; Asym; 1; #1"};
    createFittedWorkspacesGroup(label, inputNames, true);
    const std::string baseName = "MuonSimulFit_" + label;
    m_presenter->handleFittedWorkspaces(baseName + "_MUSR15189", baseName);
    // extracted = false as we have not called extractFittedWorkspaces
    // differentGroupName = true as group is different to base name
    checkFittedWorkspacesHandledCorrectly(label, inputNames, false, true);
  }

  void test_handleFittedWorkspaces_cannotFindWorkspaces_throws() {
    const std::string baseName("MuonSimulFit_UniqueLabelThatIsNotInTheADS");
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(baseName));
    TS_ASSERT_THROWS(m_presenter->handleFittedWorkspaces(baseName),
                     const Mantid::Kernel::Exception::NotFoundError &);
  }

  void test_extractFittedWorkspaces_defaultGroupName() {
    // Set up workspaces
    auto &ads = AnalysisDataService::Instance();
    auto &wsf = WorkspaceFactory::Instance();
    const std::string baseName = "MuonSimulFit_Label";
    const std::string groupName = "MuonSimulFit_Label_Workspaces";
    ads.add(baseName, boost::make_shared<WorkspaceGroup>());
    ads.add(groupName, boost::make_shared<WorkspaceGroup>());
    ads.addToGroup(baseName, groupName);
    constexpr size_t nWorkspaces = 3;
    std::array<std::string, nWorkspaces> workspaceNames;
    for (size_t i = 0; i < nWorkspaces; i++) {
      const std::string name = baseName + "_Workspace" + std::to_string(i);
      workspaceNames[i] = name;
      const auto matrixWs = wsf.create("Workspace2D", 1, 1, 1);
      const auto ws = boost::dynamic_pointer_cast<Workspace>(matrixWs);
      ads.add(name, ws);
      ads.addToGroup(groupName, name);
    }
    // Perform extraction and test results
    m_presenter->extractFittedWorkspaces(baseName);
    TS_ASSERT(ads.doesExist(groupName) == false);
    const auto baseWS = ads.retrieveWS<WorkspaceGroup>(baseName);
    TS_ASSERT(baseWS);
    for (const auto &name : workspaceNames) {
      TS_ASSERT(baseWS->contains(name));
    }
  }

  void test_extractFittedWorkspaces_otherGroupName() {
    // Set up workspaces
    auto &ads = AnalysisDataService::Instance();
    auto &wsf = WorkspaceFactory::Instance();
    const std::string outerName = "MuonSimulFit_Label";
    const std::string baseName = outerName + "_MUSR15189";
    const std::string innerName = baseName + "_Workspaces";
    ads.add(outerName, boost::make_shared<WorkspaceGroup>());
    ads.add(innerName, boost::make_shared<WorkspaceGroup>());
    ads.addToGroup(outerName, innerName);
    constexpr size_t nWorkspaces = 3;
    std::array<std::string, nWorkspaces> workspaceNames;
    for (size_t i = 0; i < nWorkspaces; i++) {
      const std::string name = baseName + "_Workspace" + std::to_string(i);
      workspaceNames[i] = name;
      const auto matrixWs = wsf.create("Workspace2D", 1, 1, 1);
      const auto ws = boost::dynamic_pointer_cast<Workspace>(matrixWs);
      ads.add(name, ws);
      ads.addToGroup(innerName, name);
    }
    // Perform extraction and test results
    m_presenter->extractFittedWorkspaces(baseName, outerName);
    TS_ASSERT(ads.doesExist(innerName) == false);
    const auto baseWS = ads.retrieveWS<WorkspaceGroup>(outerName);
    TS_ASSERT(baseWS);
    for (const auto &name : workspaceNames) {
      TS_ASSERT(baseWS->contains(name));
    }
  }

  void test_extractFittedWorkspaces_cannotFindWorkspaces_throws() {
    const std::string baseName = "MuonSimulFit_UniqueLabelThatIsNotInTheADS";
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(baseName));
    TS_ASSERT_THROWS(m_presenter->extractFittedWorkspaces(baseName),
                     const Mantid::Kernel::Exception::NotFoundError &);
  }

  void test_checkAndUpdateFitLabel_Simultaneous_NoOverwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::Simultaneous,
                                  {"fwd"}, {"1"}, false, true);
  }

  void test_checkAndUpdateFitLabel_Simultaneous_Overwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::Simultaneous,
                                  {"fwd"}, {"1"}, true, true);
  }

  void test_checkAndUpdateFitLabel_SingleRun_NoOverwrite_NoUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::Single,
                                  {"fwd"}, {"1"}, false, false);
  }

  void test_checkAndUpdateFitLabel_CoAdd_NoOverwrite_NoUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::CoAdd, {"fwd"},
                                  {"1"}, false, false);
  }

  void
  test_checkAndUpdateFitLabel_SingleRun_MultipleGroups_NoOverwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::Single,
                                  {"fwd", "bwd"}, {"1"}, false, true);
  }

  void
  test_checkAndUpdateFitLabel_SingleRun_MultipleGroups_Overwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::Single,
                                  {"fwd", "bwd"}, {"1"}, true, true);
  }

  void
  test_checkAndUpdateFitLabel_SingleRun_MultiplePeriods_NoOverwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::Single,
                                  {"fwd"}, {"1", "2"}, false, true);
  }

  void
  test_checkAndUpdateFitLabel_SingleRun_MultiplePeriods_Overwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::Single,
                                  {"fwd"}, {"1", "2"}, true, true);
  }

  void
  test_checkAndUpdateFitLabel_CoAdd_MultipleGroups_NoOverwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::CoAdd,
                                  {"fwd", "bwd"}, {"1"}, false, true);
  }

  void
  test_checkAndUpdateFitLabel_CoAdd_MultipleGroups_Overwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::CoAdd,
                                  {"fwd", "bwd"}, {"1"}, true, true);
  }

  void
  test_checkAndUpdateFitLabel_CoAdd_MultiplePeriods_NoOverwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::CoAdd, {"fwd"},
                                  {"1", "2"}, false, true);
  }
  void
  test_checkAndUpdateFitLabel_CoAdd_MultiplePeriods_Overwrite_ShouldUpdate() {
    doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType::CoAdd, {"fwd"},
                                  {"1", "2"}, true, true);
  }

  void test_handleFitRawData_NoUpdate() {
    const bool isRawData = true;
    const bool updateWorkspaces = false;
    EXPECT_CALL(*m_dataSelector, getInstrumentName()).Times(0);
    EXPECT_CALL(*m_dataSelector, getChosenGroups()).Times(0);
    EXPECT_CALL(*m_dataSelector, getPeriodSelections()).Times(0);
    EXPECT_CALL(*m_dataSelector, getFitType()).Times(0);
    m_presenter->handleFitRawData(isRawData, updateWorkspaces);
    const auto &workspaces = AnalysisDataService::Instance().getObjectNames();
    TS_ASSERT(workspaces.empty());
  }

  void test_handleFitRawData_updateWorkspaces() {
    const bool isRawData = true;
    const bool updateWorkspaces = true;
    EXPECT_CALL(*m_dataSelector, getInstrumentName())
        .Times(1)
        .WillOnce(Return("MUSR"));
    EXPECT_CALL(*m_dataSelector, getChosenGroups())
        .Times(1)
        .WillOnce(Return(QStringList({"long"})));
    EXPECT_CALL(*m_dataSelector, getPeriodSelections())
        .Times(1)
        .WillOnce(Return(QStringList({"1"})));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(IMuonFitDataSelector::FitType::Single));
    ON_CALL(*m_dataSelector, getRuns()).WillByDefault(Return("15189"));
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.55));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(10.0));
    const QStringList expectedNames{
        "MUSR00015189; Pair; long; Asym; 1; #1_Raw"};
    EXPECT_CALL(*m_fitBrowser, setWorkspaceNames(expectedNames)).Times(1);
    EXPECT_CALL(*m_dataSelector, setDatasetNames(expectedNames)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setWorkspaceName(expectedNames[0])).Times(1);
    EXPECT_CALL(*m_fitBrowser, allowSequentialFits(true));
    m_presenter->handleFitRawData(isRawData, updateWorkspaces);
    TS_ASSERT(AnalysisDataService::Instance().doesExist(
        expectedNames[0].toStdString()));
  }

  void test_checkAndUpdateFitLabel_SequentialFit_ShouldDoNothing() {
    EXPECT_CALL(*m_dataSelector, getFitType()).Times(0);
    EXPECT_CALL(*m_dataSelector, getChosenGroups()).Times(0);
    EXPECT_CALL(*m_dataSelector, getPeriodSelections()).Times(0);
    EXPECT_CALL(*m_dataSelector, askUserWhetherToOverwrite()).Times(0);
    EXPECT_CALL(*m_fitBrowser, setSimultaneousLabel(_)).Times(0);
    EXPECT_CALL(*m_dataSelector, setSimultaneousFitLabel(_)).Times(0);
    m_presenter->checkAndUpdateFitLabel(true);
  }

  void test_setSelectedWorkspace() {
    setupGroupPeriodSelections();
    const QString wsName("MUSR00015189-91; Group; fwd; Asym; 1; #6");
    const QStringList wsNameList{wsName};

    // Expect it will update the workspace names
    EXPECT_CALL(*m_fitBrowser, setWorkspaceNames(wsNameList)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setWorkspaceName(wsName)).Times(1);
    EXPECT_CALL(*m_dataSelector, setDatasetNames(wsNameList)).Times(1);

    // Expect it will update the UI from workspace details
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00015189-91"), QString("MUSR"),
                                    Eq(boost::none)))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setGroupsSelected(QStringList({"fwd"})))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setPeriodsSelected(QStringList({"1"})))
        .Times(1);

    localSetSelectedWorkspace(wsName, boost::none);
  }
  void test_setSelectedWorkspace_loadCurrentRun() {
    setupGroupPeriodSelections();
    const QString wsName("MUSR00061335; Group; fwd; Asym; 1; #1");
    const QStringList wsNameList{wsName};
    const boost::optional<QString> currentRunPath{
        R"(\\musr\data\MUSRauto_A.tmp)"};

    // Expect it will update the workspace names
    EXPECT_CALL(*m_fitBrowser, setWorkspaceNames(wsNameList)).Times(1);
    EXPECT_CALL(*m_fitBrowser, setWorkspaceName(wsName)).Times(1);
    EXPECT_CALL(*m_dataSelector, setDatasetNames(wsNameList)).Times(1);

    // Expect it will update the UI from workspace details
    EXPECT_CALL(*m_dataSelector,
                setWorkspaceDetails(QString("00061335"), QString("MUSR"),
                                    Eq(currentRunPath)))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setGroupsSelected(QStringList({"fwd"})))
        .Times(1);
    EXPECT_CALL(*m_dataSelector, setPeriodsSelected(QStringList({"1"})))
        .Times(1);

    localSetSelectedWorkspace(wsName, currentRunPath);
  }

  void test_doPreFitChecks_nonSequential_invalidRuns_doesNotFit() {
    const QString invalidRuns("");
    doTest_doPreFitChecks(false, invalidRuns, false);
  }

  void test_doPreFitChecks_nonSequential_validRuns_doesFit() {
    const QString validRuns("15189-91");
    doTest_doPreFitChecks(false, validRuns, true);
  }

  void test_doPreFitChecks_sequential_invalidRuns_doesNotFit() {
    const QString invalidRuns("");
    doTest_doPreFitChecks(true, invalidRuns, false);
  }

  void test_doPreFitChecks_sequential_validRuns_doesFit() {
    const QString validRuns("15189-91");
    doTest_doPreFitChecks(true, validRuns, true);
  }

private:
  void
  doTest_generateWorkspaceNames(const IMuonFitDataSelector::FitType &fitType,
                                const bool isRawData,
                                const std::vector<std::string> &expectedNames) {
    EXPECT_CALL(*m_dataSelector, getChosenGroups())
        .Times(1)
        .WillOnce(Return(QStringList({"long"})));
    EXPECT_CALL(*m_dataSelector, getPeriodSelections())
        .Times(1)
        .WillOnce(Return(QStringList({"1"})));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(fitType));
    m_presenter->handleFitRawData(isRawData,
                                  false); // don't create the workspaces
    const auto &names =
        m_presenter->generateWorkspaceNames("MUSR", "15189-91", true);
    TS_ASSERT_EQUALS(names, expectedNames)
  }

  void doTest_handleSelectedDataChanged(IMuonFitDataSelector::FitType fitType) {
    auto &ads = AnalysisDataService::Instance();
    EXPECT_CALL(*m_dataSelector, getInstrumentName())
        .Times(1)
        .WillOnce(Return("MUSR"));
    EXPECT_CALL(*m_dataSelector, getChosenGroups())
        .Times(1)
        .WillOnce(Return(QStringList({"fwd", "long"})));
    EXPECT_CALL(*m_dataSelector, getPeriodSelections())
        .Times(1)
        .WillOnce(Return(QStringList({"1", "1-2"})));
    EXPECT_CALL(*m_dataSelector, getFitType())
        .Times(1)
        .WillOnce(Return(fitType));
    ON_CALL(*m_dataSelector, getRuns()).WillByDefault(Return("15189-91"));
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.55));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(10.0));
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return(QString("UserSelectedFitLabel")));
    const std::vector<QString> expectedNames = [&fitType]() {
      if (fitType == IMuonFitDataSelector::FitType::CoAdd) {
        return std::vector<QString>{
            "MUSR00015189-91; Group; fwd; Asym; 1; #1",
            "MUSR00015189-91; Pair; long; Asym; 1; #1",
            "MUSR00015189-91; Group; fwd; Asym; 1-2; #1",
            "MUSR00015189-91; Pair; long; Asym; 1-2; #1"};
      } else {
        return std::vector<QString>{"MUSR00015189; Group; fwd; Asym; 1; #1",
                                    "MUSR00015189; Pair; long; Asym; 1; #1",
                                    "MUSR00015189; Group; fwd; Asym; 1-2; #1",
                                    "MUSR00015189; Pair; long; Asym; 1-2; #1",
                                    "MUSR00015190; Group; fwd; Asym; 1; #1",
                                    "MUSR00015190; Pair; long; Asym; 1; #1",
                                    "MUSR00015190; Group; fwd; Asym; 1-2; #1",
                                    "MUSR00015190; Pair; long; Asym; 1-2; #1",
                                    "MUSR00015191; Group; fwd; Asym; 1; #1",
                                    "MUSR00015191; Pair; long; Asym; 1; #1",
                                    "MUSR00015191; Group; fwd; Asym; 1-2; #1",
                                    "MUSR00015191; Pair; long; Asym; 1-2; #1"};
      }
    }();
    EXPECT_CALL(*m_fitBrowser,
                setWorkspaceNames(UnorderedElementsAreArray(expectedNames)))
        .Times(1);
    EXPECT_CALL(*m_dataSelector,
                setDatasetNames(UnorderedElementsAreArray(expectedNames)))
        .Times(1);
    EXPECT_CALL(*m_fitBrowser, setWorkspaceName(_)).Times(1);
    EXPECT_CALL(*m_fitBrowser, allowSequentialFits(false));
    m_dataLoader.setDeadTimesType(DeadTimesType::FromFile);
    ads.add("MUSR00015189", boost::make_shared<WorkspaceGroup>());
    m_presenter->handleSelectedDataChanged(true);
    // test that all expected names are in the ADS
    const auto namesInADS = ads.getObjectNames();
    for (const QString &name : expectedNames) {
      TS_ASSERT(std::find(namesInADS.begin(), namesInADS.end(),
                          name.toStdString()) != namesInADS.end());
    }
    // test that workspaces have been added to correct groups
    auto existingGroup = ads.retrieveWS<WorkspaceGroup>("MUSR00015189");
    TS_ASSERT(existingGroup);
    // Simultaneous case
    if (fitType == IMuonFitDataSelector::FitType::Simultaneous) {
      if (existingGroup) {
        for (int i = 0; i < 4; i++) {
          TS_ASSERT(existingGroup->contains(expectedNames[i].toStdString()));
        }
      }
      auto newGroup = ads.retrieveWS<WorkspaceGroup>("MUSR00015190");
      TS_ASSERT(newGroup);
      if (newGroup) {
        for (int i = 4; i < 8; i++) {
          TS_ASSERT(newGroup->contains(expectedNames[i].toStdString()));
        }
      }
    } else {
      // Coadd case
      auto newGroup = ads.retrieveWS<WorkspaceGroup>("MUSR00015189-91");
      TS_ASSERT(newGroup);
      if (newGroup) {
        for (const auto &name : expectedNames) {
          TS_ASSERT(newGroup->contains(name.toStdString()));
        }
      }
    }
  }

  /**
   * Creates a group of workspaces that are the output of a simultaneous fit,
   * that handleFitFinished() will act on, e.g.:
   *
   * MuonSimulFit_Label
   *   \__MuonSimulFit_Label_Parameters
   *   \__MuonSimulFit_Label_Workspaces
   *         \__MuonSimulFit_Label_Workspace0
   *         \__MuonSimulFit_Label_Workspace1
   *         \__ ...
   *
   * @param label :: [input] Selected label for fit
   * @param inputNames :: [input] Names of input workspaces
   * @param differentGroupName :: [input] Set true if workspace base name is not
   * the same as group name
   */
  void createFittedWorkspacesGroup(const std::string &label,
                                   const std::vector<std::string> &inputNames,
                                   bool differentGroupName = false) {
    auto &ads = AnalysisDataService::Instance();
    auto &wsf = WorkspaceFactory::Instance();
    const std::string groupName = "MuonSimulFit_" + label;
    const std::string baseName =
        differentGroupName ? groupName + "_MUSR15189" : groupName;
    const std::string wsGroupName = baseName + "_Workspaces";
    const std::string paramName = baseName + "_Parameters";
    const std::string ncmName = groupName + "_NormalisedCovarianceMatrix";
    ads.add(groupName, boost::make_shared<WorkspaceGroup>());
    ads.add(wsGroupName, boost::make_shared<WorkspaceGroup>());
    ads.addToGroup(groupName, wsGroupName);
    auto paramTable = wsf.createTable();
    paramTable->addColumn("str", "Name");
    paramTable->addColumn("double", "Value");
    paramTable->addColumn("double", "Error");
    for (size_t i = 0; i < inputNames.size(); i++) {
      const std::string name = baseName + "_Workspace" + std::to_string(i);
      const auto matrixWs = wsf.create("Workspace2D", 1, 1, 1);
      const auto ws = boost::dynamic_pointer_cast<Workspace>(matrixWs);
      ads.add(name, ws);
      ads.addToGroup(wsGroupName, name);
      TableRow rowA0 = paramTable->appendRow();
      TableRow rowA1 = paramTable->appendRow();
      rowA0 << "f" + std::to_string(i) + ".A0" << 0.1 << 0.01;
      rowA1 << "f" + std::to_string(i) + ".A1" << 0.2 << 0.02;
    }
    TableRow costFuncRow = paramTable->appendRow();
    costFuncRow << "Cost function value" << 1.0 << 0.0;
    for (size_t i = 0; i < inputNames.size(); i++) {
      TableRow row = paramTable->appendRow();
      std::ostringstream oss;
      oss << "f" << std::to_string(i) << "=" << inputNames[i];
      row << oss.str() << 0.0 << 0.0;
    }
    ads.add(paramName, paramTable);
    ads.addToGroup(groupName, paramName);
    const auto ncmWs = boost::dynamic_pointer_cast<Workspace>(
        wsf.create("Workspace2D", 1, 1, 1));
    ads.add(ncmName, ncmWs);
    ads.addToGroup(groupName, ncmName);
  }

  /**
   * Checks the results of handleFitFinished() to see if workspaces are dealt
   * with correctly:
   *
   * MuonSimulFit_Label
   *   \__MuonSimulFit_Label_MUSR00015189_long_1_Workspace
   *   \__...
   *   \__MuonSimulFit_Label_MUSR00015189_long_1_Parameters
   *   \__...
   *
   * @param label :: [input] Selected label for fit
   * @param inputNames :: [input] Names of input workspaces
   * @param extracted :: [input] Whether workspaces should have been extracted
   * from "_Workspaces" group or not
   * @param differentGroupName :: [input] Set true if workspace base name is not
   * the same as group name
   */
  void checkFittedWorkspacesHandledCorrectly(
      const std::string &label, const std::vector<std::string> &inputNames,
      bool extracted = true, bool differentGroupName = false) {
    auto &ads = AnalysisDataService::Instance();

    const std::string groupName = "MuonSimulFit_" + label;
    const std::string baseName =
        differentGroupName ? groupName + "_MUSR15189" : groupName;
    const auto baseGroup = ads.retrieveWS<WorkspaceGroup>(groupName);
    TS_ASSERT(baseGroup);
    if (baseGroup) {
      // generate expected names
      std::vector<std::string> expectedNames{groupName +
                                             "_NormalisedCovarianceMatrix"};
      if (!extracted) {
        expectedNames.push_back(baseName + "_Workspaces");
      }
      for (const auto &name : inputNames) {
        std::ostringstream oss;
        const auto wsParams =
            MantidQt::CustomInterfaces::MuonAnalysisHelper::parseWorkspaceName(
                name);
        oss << baseName << "_" << wsParams.label << "_" << wsParams.itemName
            << "_" << wsParams.periods;
        if (extracted) {
          expectedNames.push_back(oss.str() + "_Workspace");
        }
        expectedNames.push_back(oss.str() + "_Parameters");
      }
      // Check expected workspaces in group
      auto groupNames(baseGroup->getNames());
      std::sort(groupNames.begin(), groupNames.end());
      std::sort(expectedNames.begin(), expectedNames.end());
      TS_ASSERT_EQUALS(expectedNames, groupNames);

      // Check parameter tables
      for (size_t i = 0; i < baseGroup->size(); i++) {
        const auto table =
            boost::dynamic_pointer_cast<ITableWorkspace>(baseGroup->getItem(i));
        if (table) {
          auto columns = table->getColumnNames();
          std::sort(columns.begin(), columns.end());
          TS_ASSERT_EQUALS(
              columns, std::vector<std::string>({"Error", "Name", "Value"}));
          TS_ASSERT_EQUALS(table->rowCount(), 3);
          TS_ASSERT_EQUALS(table->String(0, 0), "A0");
          TS_ASSERT_EQUALS(table->String(1, 0), "A1");
          TS_ASSERT_EQUALS(table->String(2, 0), "Cost function value");
          TS_ASSERT_EQUALS(table->Double(0, 1), 0.1);
          TS_ASSERT_EQUALS(table->Double(1, 1), 0.2);
          TS_ASSERT_EQUALS(table->Double(2, 1), 1.0);
          TS_ASSERT_EQUALS(table->Double(0, 2), 0.01);
          TS_ASSERT_EQUALS(table->Double(1, 2), 0.02);
          TS_ASSERT_EQUALS(table->Double(2, 2), 0.0);
        }
      }
    }
  }

  /**
   * Test checkAndUpdateFitLabel() with the given options
   * @param fitType :: [input] Type of fit
   * @param groups :: [input] Groups to fit
   * @param periods :: [input] Periods to fit
   * @param overwrite :: [input] Whether user chose to overwrite
   * @param shouldUpdate :: [input] Whether or not to expect label to be updated
   */
  void doTest_checkAndUpdateFitLabel(IMuonFitDataSelector::FitType fitType,
                                     const QStringList &groups,
                                     const QStringList &periods, bool overwrite,
                                     bool shouldUpdate) {
    ON_CALL(*m_dataSelector, getFitType()).WillByDefault(Return(fitType));
    ON_CALL(*m_dataSelector, getChosenGroups()).WillByDefault(Return(groups));
    ON_CALL(*m_dataSelector, getPeriodSelections())
        .WillByDefault(Return(periods));
    ON_CALL(*m_dataSelector, askUserWhetherToOverwrite())
        .WillByDefault(Return(overwrite));
    ON_CALL(*m_dataSelector, getSimultaneousFitLabel())
        .WillByDefault(Return("UserSelectedFitLabel"));

    if (shouldUpdate) {
      const QString label = "UserSelectedFitLabel";
      QString groupName = QString("MuonSimulFit_").append(label);
      AnalysisDataService::Instance().add(groupName.toStdString(),
                                          boost::make_shared<WorkspaceGroup>());
      const auto &uniqueName = overwrite ? label : label + "#2";
      EXPECT_CALL(*m_fitBrowser, setSimultaneousLabel(uniqueName.toStdString()))
          .Times(1);
      EXPECT_CALL(*m_dataSelector, setSimultaneousFitLabel(uniqueName))
          .Times(1);
    } else {
      EXPECT_CALL(*m_fitBrowser, setSimultaneousLabel(_)).Times(0);
      EXPECT_CALL(*m_dataSelector, setSimultaneousFitLabel(_)).Times(0);
    }
    m_presenter->checkAndUpdateFitLabel(false);
    AnalysisDataService::Instance().clear();
  }

  void setupGroupPeriodSelections() {
    ON_CALL(*m_dataSelector, getChosenGroups())
        .WillByDefault(Return(QStringList{}));
    ON_CALL(*m_dataSelector, getPeriodSelections())
        .WillByDefault(Return(QStringList{}));
  }

  void setupForDataChange() {
    ON_CALL(*m_dataSelector, getChosenGroups())
        .WillByDefault(Return(QStringList{"fwd"}));
    ON_CALL(*m_dataSelector, getPeriodSelections())
        .WillByDefault(Return(QStringList{"1"}));
    ON_CALL(*m_dataSelector, getFitType())
        .WillByDefault(Return(IMuonFitDataSelector::FitType::Simultaneous));
    ON_CALL(*m_dataSelector, getInstrumentName()).WillByDefault(Return("MUSR"));
    ON_CALL(*m_dataSelector, getRuns()).WillByDefault(Return("15189-91"));
    ON_CALL(*m_dataSelector, getStartTime()).WillByDefault(Return(0.55));
    ON_CALL(*m_dataSelector, getEndTime()).WillByDefault(Return(10.0));
  }

  void doTest_doPreFitChecks(bool sequential, const QString &runString,
                             bool willFit) {
    setupGroupPeriodSelections();
    ON_CALL(*m_dataSelector, getFitType())
        .WillByDefault(Return(IMuonFitDataSelector::FitType::Single));
    EXPECT_CALL(*m_dataSelector, getRuns())
        .Times(1)
        .WillOnce(Return(runString));
    if (willFit) {
      EXPECT_CALL(*m_fitBrowser, continueAfterChecks(sequential)).Times(1);
    } else {
      EXPECT_CALL(*m_fitBrowser, continueAfterChecks(_)).Times(0);
    }
    m_presenter->doPreFitChecks(sequential);
  }

  /// method to manually set up workspace
  /// this is a work around for the signal/slots
  void localSetAssignedFirstRun(const QString &wsName,
                                const boost::optional<QString> &filepath) {
    m_presenter->setAssignedFirstRun(wsName, filepath);
    // manually replicate signal
    const auto wsParams =
        MantidQt::CustomInterfaces::MuonAnalysisHelper::parseWorkspaceName(
            wsName.toStdString());
    m_dataSelector->setPeriodsSelected(
        QStringList{QString::fromStdString(wsParams.periods)});
    m_dataSelector->setGroupsSelected(
        QStringList{QString::fromStdString(wsParams.itemName)});
  }

  /// method to manually set up workspace
  /// this is a work around for the signal/slots
  void localSetSelectedWorkspace(const QString &wsName,
                                 const boost::optional<QString> &filepath) {
    m_presenter->setSelectedWorkspace(wsName, filepath);
    // manually replicate signal
    const auto wsParams =
        MantidQt::CustomInterfaces::MuonAnalysisHelper::parseWorkspaceName(
            wsName.toStdString());
    m_dataSelector->setPeriodsSelected(
        QStringList{QString::fromStdString(wsParams.periods)});
    m_dataSelector->setGroupsSelected(
        QStringList{QString::fromStdString(wsParams.itemName)});
  }

  MockDataSelector *m_dataSelector;
  MockFitBrowser *m_fitBrowser;
  MuonAnalysisFitDataPresenter *m_presenter;
  MuonAnalysisDataLoader m_dataLoader;
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAPRESENTERTEST_H_ */
