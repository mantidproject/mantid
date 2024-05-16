// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../ReflMockObjects.h"
#include "GUI/Save/IFileSaver.h"
#include "GUI/Save/SavePresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MockSaveView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using Mantid::API::AlgorithmManager;
using Mantid::API::AnalysisDataService;
using Mantid::DataObjects::Workspace2D_sptr;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class SavePresenterTest : public CxxTest::TestSuite {
public:
  static SavePresenterTest *createSuite() { return new SavePresenterTest(); }
  static void destroySuite(SavePresenterTest *suite) { delete suite; }

  SavePresenterTest()
      : m_view(), m_savePath("/foo/bar/"), m_fileFormat(NamedFormat::Custom), m_prefix("testoutput_"),
        m_includeHeader(true), m_separator(","), m_includeQResolution(true), m_includeAdditionalColumns(false) {}

  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
  }

  void testSetWorkspaceListOnConstruction() {
    auto workspaceNames = createWorkspaces();
    expectSetWorkspaceListFromADS(workspaceNames);
    auto presenter = makePresenter();
  }

  void testSetDefaultSavePathOnConstruction() {
    auto const path = Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory");
    EXPECT_CALL(m_view, setSavePath(path)).Times(1);
    auto presenter = makePresenter();
  }

  void testNotifyPopulateWorkspaceList() {
    auto presenter = makePresenter();
    auto workspaceNames = createWorkspaces();
    expectSetWorkspaceListFromADS(workspaceNames);
    presenter.notifyPopulateWorkspaceList();
  }

  void testUpdateWorkspaceList() {
    auto presenter = makePresenter();
    createWorkspace("ws1");
    expectSetWorkspaceListFromADS({"ws1"});
    presenter.notifyPopulateWorkspaceList();
    createWorkspace("ws2");
    expectSetWorkspaceListFromADS({"ws1", "ws2"});
    presenter.notifyPopulateWorkspaceList();
  }

  void testNotifyPopulateWorkspaceListExcludesInvalidWorkspaceTypes() {
    auto presenter = makePresenter();
    // Create some valid workspaces
    createWorkspaces({"ws1", "ws2"});
    // Create a table workspace
    createTableWorkspace("tableWS");
    // Group workspaces 1 and 2 together
    createWorkspaceGroup("groupWS", {"ws3", "ws4"});
    // "tableWS" and "groupWS" should not be included in the workspace list
    expectSetWorkspaceListFromADS({"ws1", "ws2", "ws3", "ws4"});
    presenter.notifyPopulateWorkspaceList();
  }

  void testNotifyFilterWorkspaceList() {
    auto presenter = makePresenter();
    auto const filter = std::string("Ws");
    auto const inputWorkspaces = std::vector<std::string>{"someWsName", "different", "anotherWs"};
    auto const filteredWorkspaces = std::vector<std::string>{"anotherWs", "someWsName"};
    createWorkspaces(inputWorkspaces);
    EXPECT_CALL(m_view, getFilter()).Times(1).WillOnce(Return(filter));
    EXPECT_CALL(m_view, getRegexCheck()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(filteredWorkspaces)).Times(1);
    presenter.notifyFilterWorkspaceList();
  }

  void testNotifyFilterWorkspaceListByRegex() {
    auto presenter = makePresenter();
    auto const filter = std::string("[a-zA-Z]*_[0-9]+");
    auto const inputWorkspaces = std::vector<std::string>{"_42", "apple_113", "grape_", "pear_cut"};
    auto const filteredWorkspaces = std::vector<std::string>{"_42", "apple_113"};
    createWorkspaces(inputWorkspaces);
    EXPECT_CALL(m_view, getFilter()).Times(1).WillOnce(Return(filter));
    EXPECT_CALL(m_view, getRegexCheck()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(m_view, showFilterEditValid()).Times(1);
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(filteredWorkspaces)).Times(1);
    presenter.notifyFilterWorkspaceList();
  }

  void testNotifyFilterWorkspaceListWithInvalidRegex() {
    auto presenter = makePresenter();
    auto const filter = std::string("w[.*kspace");
    auto const inputWorkspaces = std::vector<std::string>{"first_test_workspace", "test_ws_2", "dummy_wkspace"};
    auto const filteredWorkspaces = std::vector<std::string>{};
    createWorkspaces(inputWorkspaces);
    EXPECT_CALL(m_view, getFilter()).Times(1).WillOnce(Return(filter));
    EXPECT_CALL(m_view, getRegexCheck()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(m_view, showFilterEditInvalid()).Times(1);
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(filteredWorkspaces)).Times(1);
    presenter.notifyFilterWorkspaceList();
  }

  void testNotifyPopulateParametersList() {
    auto presenter = makePresenter();
    // Add some workspaces without logs
    createWorkspaces({"test1", "test2"});
    // Add a workspace with a Theta log value, which we'll get the view return
    // as the current workspace
    auto const currentWorkspace = std::string("test3");
    createWorkspacesWithThetaLog({currentWorkspace});
    auto const expectedLogs = std::vector<std::string>{"Theta"};
    EXPECT_CALL(m_view, clearParametersList()).Times(1);
    EXPECT_CALL(m_view, getCurrentWorkspaceName()).Times(1).WillOnce(Return(currentWorkspace));
    EXPECT_CALL(m_view, setParametersList(expectedLogs)).Times(1);
    presenter.notifyPopulateParametersList();
  }

  void testNotifyPopulateParametersListWithWorkspaceNotInADS() {
    auto presenter = makePresenter();
    auto const workspaceName = "test";
    EXPECT_CALL(m_view, clearParametersList()).Times(1);
    EXPECT_CALL(m_view, getCurrentWorkspaceName()).Times(1).WillOnce(Return(workspaceName));
    EXPECT_CALL(m_view, setParametersList(_)).Times(0);
    presenter.notifyPopulateParametersList();
  }

  void testNotifySaveSelectedWorkspacesWithLogs() {
    auto presenter = makePresenter();
    auto const inputWorkspaces = std::vector<std::string>{"test1", "test2", "test3", "test4"};
    createWorkspacesWithThetaLog(inputWorkspaces);
    auto const logs = std::vector<std::string>{"Theta"};
    auto selectedWorkspaces = std::vector<std::string>{"test2", "test4"};
    EXPECT_CALL(m_view, getSelectedWorkspaces()).Times(1).WillOnce(Return(selectedWorkspaces));
    expectSaveWorkspaces(selectedWorkspaces, logs);
    presenter.notifySaveSelectedWorkspaces();
  }

  void testNotifySaveSelectedWorkspacesWhenNothingSelected() {
    auto presenter = makePresenter();
    auto emptyWorkspaceList = std::vector<std::string>{};
    EXPECT_CALL(m_view, getSelectedWorkspaces()).Times(1).WillOnce(Return(emptyWorkspaceList));
    EXPECT_CALL(m_view, noWorkspacesSelected()).Times(1);
    presenter.notifySaveSelectedWorkspaces();
  }

  void testNotifySaveSelectedWorkspacesIgnoresSingleFileCheckbox() {
    auto presenter = makePresenter();
    auto const inputWorkspaces = std::vector<std::string>{"test1", "test2", "test3", "test4"};
    createWorkspaces(inputWorkspaces);
    auto selectedWorkspaces = std::vector<std::string>{"test2", "test4"};
    EXPECT_CALL(m_view, getSelectedWorkspaces()).Times(1).WillOnce(Return(selectedWorkspaces));
    expectSaveWorkspacesNoLogs(selectedWorkspaces, true, false, false);
    presenter.notifySaveSelectedWorkspaces();
  }

  void testSaveWorkspacesWithNoAutoSaveIgnoresSingleFileCheckbox() { runSaveWorkspacesTest(true, false, false); }

  void testSaveWorkspacesWithAutoSaveAndSingleFileCheckboxSelected() { runSaveWorkspacesTest(true, true, true); }

  void testSaveWorkspacesWithAutoSaveAndSingleFileCheckboxNotSelected() { runSaveWorkspacesTest(false, true, false); }

  void testNotifyAutosaveDisabled() {
    auto presenter = makePresenter();
    // There are no calls to the view
    EXPECT_CALL(m_view, disableSaveIndividualRowsCheckbox()).Times(1);
    EXPECT_CALL(m_view, disableSaveToSingleFileCheckBox()).Times(1);
    presenter.notifyAutosaveDisabled();
  }

  void testNotifyAutosaveEnabledForCustomFormat() { checkNotifyAutosaveEnabledForFormat(NamedFormat::Custom, false); }

  void testNotifyAutosaveEnabledForILLCosmosFormat() {
    checkNotifyAutosaveEnabledForFormat(NamedFormat::ILLCosmos, false);
  }

  void testNotifyAutosaveEnabledForANSTOFormat() { checkNotifyAutosaveEnabledForFormat(NamedFormat::ANSTO, false); }

  void testNotifyAutosaveEnabledForThreeColumnFormat() {
    checkNotifyAutosaveEnabledForFormat(NamedFormat::ThreeColumn, false);
  }

  void testNotifyAutosaveEnabledForORSOAsciiFormat() {
    checkNotifyAutosaveEnabledForFormat(NamedFormat::ORSOAscii, true);
  }

  void testNotifyAutosaveEnabledForORSONexusFormat() {
    checkNotifyAutosaveEnabledForFormat(NamedFormat::ORSONexus, true);
  }

  void testNotifyAutosaveEnabledWithInvalidPath() {
    auto presenter = makePresenter();
    expectGetInvalidSaveDirectory();
    EXPECT_CALL(m_view, enableSaveIndividualRowsCheckbox()).Times(0);
    EXPECT_CALL(m_view, enableSaveToSingleFileCheckBox()).Times(0);
    EXPECT_CALL(m_view, disallowAutosave()).Times(1);
    EXPECT_CALL(m_view, errorInvalidSaveDirectory()).Times(1);
    presenter.notifyAutosaveEnabled();
  }

  void testNotifySaveIndividualRowsEnabled() {
    auto presenter = makePresenter();
    // There are no calls to the view
    presenter.notifySaveIndividualRowsEnabled();
  }

  void testNotifySaveIndividualRowsDisabled() {
    auto presenter = makePresenter();
    // There are no calls to the view
    presenter.notifySaveIndividualRowsDisabled();
  }

  void testShouldAutosaveGroupRowsFalseByDefault() {
    auto presenter = makePresenter();
    bool saveRows = presenter.shouldAutosaveGroupRows();
    TS_ASSERT(!saveRows);
  }

  void testShouldAutosaveGroupRowsWhenSaveIndividualRowsIsEnabled() {
    auto presenter = makePresenter();
    presenter.notifySaveIndividualRowsEnabled();
    bool saveRows = presenter.shouldAutosaveGroupRows();
    TS_ASSERT(saveRows);
  }

  void testShouldAutosaveGroupRowsWhenSaveIndividualRowsIsDisabled() {
    auto presenter = makePresenter();
    presenter.notifySaveIndividualRowsDisabled();
    bool saveRows = presenter.shouldAutosaveGroupRows();
    TS_ASSERT(!saveRows);
  }

  void testNotifySavePathChangedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectGetValidSaveDirectory();
    presenter.notifySavePathChanged();
  }

  void testNotifySavePathChangedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    EXPECT_CALL(m_view, getSavePath()).Times(0);
    presenter.notifySavePathChanged();
  }

  void testNotifySavePathChangedWithInvalidPath() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectGetInvalidSaveDirectory();
    EXPECT_CALL(m_view, warnInvalidSaveDirectory()).Times(1);
    presenter.notifySavePathChanged();
  }

  void testControlsEnabledWhenReductionPaused() {
    auto presenter = makePresenter();
    auto workspaceNames = createWorkspaces();
    expectSetWorkspaceListFromADS(workspaceNames);
    expectNotProcessingOrAutoreducing();
    EXPECT_CALL(m_view, enableAutosaveControls()).Times(1);
    expectFileFormatAndLocationControlsEnabled();
    presenter.notifyReductionPaused();
  }

  void testAutosaveControlsDisabledWhenReductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectProcessing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.notifyReductionResumed();
  }

  void testFileControlsDisabledWhenReductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectProcessing();
    expectFileFormatAndLocationControlsDisabled();
    presenter.notifyReductionResumed();
  }

  void testFileControlsEnabledWhenReductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectProcessing();
    expectFileFormatAndLocationControlsEnabled();
    presenter.notifyReductionResumed();
  }

  void testAutosaveControlsDisabledWhenReductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectProcessing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.notifyReductionResumed();
  }

  void testAutosaveControlsDisabledWhenAutoreductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectAutoreducing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.notifyAutoreductionResumed();
  }

  void testFileControlsDisabledWhenAutoreductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectAutoreducing();
    expectFileFormatAndLocationControlsDisabled();
    presenter.notifyAutoreductionResumed();
  }

  void testFileControlsEnabledWhenAutoreductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectAutoreducing();
    expectFileFormatAndLocationControlsEnabled();
    presenter.notifyAutoreductionResumed();
  }

  void testAutosaveControlsDisabledWhenAutoreductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectAutoreducing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.notifyAutoreductionResumed();
  }

  void testAutosaveDisabledNotifiesMainPresenter() {
    auto presenter = makePresenter();
    presenter.notifyAutosaveDisabled();
  }

  void testAutosaveEnabledNotifiesMainPresenter() {
    auto presenter = makePresenter();
    presenter.notifyAutosaveEnabled();
  }

  void testNotifyMainPresenterSettingsChanged() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, setBatchUnsaved());
    presenter.notifySettingsChanged();
  }

  // Custom format option settings

  void testLogListEnabledForCustomFormatIfHeaderEnabled() {
    auto presenter = makePresenter();
    expectFileFormat(NamedFormat::Custom);
    expectHeaderOptionEnabled();
    expectLogListEnabled();
    presenter.notifySettingsChanged();
  }

  void testLogListDisabledForCustomFormatIfHeaderDisabled() {
    auto presenter = makePresenter();
    expectFileFormat(NamedFormat::Custom);
    expectHeaderOptionDisabled();
    expectLogListDisabled();
    presenter.notifySettingsChanged();
  }

  void testCustomOptionsEnabledForCustomFormat() {
    auto presenter = makePresenter();
    expectFileFormat(NamedFormat::Custom);
    expectQResolutionEnabled();
    expectAdditionalColumnsDisabled();
    expectCustomOptionsEnabled();
    presenter.notifySettingsChanged();
  }

  void testSaveToSingleFileDisabledWithAutosaveForCustomFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::Custom, true, false);
  }

  void testSaveToSingleFileDisabledWithNoAutosaveForCustomFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::Custom, false, false);
  }

  // ILL Cosmos format option settings

  void testLogListEnabledForILLCosmosFormat() { checkLogListStateForFileFormat(NamedFormat::ILLCosmos, true); }

  void testCustomOptionsDisabledForILLCosmosFormat() {
    checkCustomOptionsStateForFileFormat(NamedFormat::ILLCosmos, false);
  }

  void testQResolutionDisabledForILLCosmosFormat() {
    checkQResolutionStateForFileFormat(NamedFormat::ILLCosmos, false);
  }

  void testAdditionalColumnsDisabledForILLCosmosFormat() {
    checkAdditionalColumnsStateForFileFormat(NamedFormat::ILLCosmos, false);
  }

  void testSaveToSingleFileDisabledWithAutosaveForILLCosmosFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ILLCosmos, true, false);
  }

  void testSaveToSingleFileDisabledWithNoAutosaveForILLCosmosFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ILLCosmos, false, false);
  }

  // ANSTO format option settings

  void testLogListDisabledForANSTOFormat() { checkLogListStateForFileFormat(NamedFormat::ANSTO, false); }

  void testCustomOptionsDisabledForANSTOFormat() { checkCustomOptionsStateForFileFormat(NamedFormat::ANSTO, false); }

  void testQResolutionDisabledForANSTOFormat() { checkQResolutionStateForFileFormat(NamedFormat::ANSTO, false); }

  void testAdditionalColumnsDisabledForANSTOFormat() {
    checkAdditionalColumnsStateForFileFormat(NamedFormat::ANSTO, false);
  }

  void testSaveToSingleFileDisabledWithAutosaveForANSTOFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ANSTO, true, false);
  }

  void testSaveToSingleFileDisabledWithNoAutosaveForANSTOFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ANSTO, false, false);
  }

  // Three Column format option settings

  void testLogListDisabledForThreeColumnFormat() { checkLogListStateForFileFormat(NamedFormat::ThreeColumn, false); }

  void testCustomOptionsDisabledForThreeColumnFormat() {
    checkCustomOptionsStateForFileFormat(NamedFormat::ThreeColumn, false);
  }

  void testQResolutionDisabledForThreeColumnFormat() {
    checkQResolutionStateForFileFormat(NamedFormat::ThreeColumn, false);
  }

  void testAdditionalColumnsDisabledForThreeColumnFormat() {
    checkAdditionalColumnsStateForFileFormat(NamedFormat::ThreeColumn, false);
  }

  void testSaveToSingleFileDisabledWithAutosaveForThreeColumnFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ThreeColumn, true, false);
  }

  void testSaveToSingleFileDisabledWithNoAutosaveForThreeColumnFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ThreeColumn, false, false);
  }

  // ORSO Ascii format option settings

  void testLogListDisabledForORSOAsciiFormat() { checkLogListStateForFileFormat(NamedFormat::ORSOAscii, false); }

  void testCustomOptionsDisabledForORSOAsciiFormat() {
    checkCustomOptionsStateForFileFormat(NamedFormat::ORSOAscii, false);
  }

  void testQResolutionEnabledForORSOAsciiFormat() { checkQResolutionStateForFileFormat(NamedFormat::ORSOAscii, true); }

  void testAdditionalColumnsEnabledForORSOAsciiFormat() {
    checkAdditionalColumnsStateForFileFormat(NamedFormat::ORSOAscii, true);
  }

  void testSaveToSingleFileEnabledWithAutosaveForORSOAsciiFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ORSOAscii, true, true);
  }

  void testSaveToSingleFileDisabledWithNoAutosaveForORSOAsciiFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ORSOAscii, false, false);
  }

  // ORSO Nexus format option settings

  void testLogListDisabledForORSONexusFormat() { checkLogListStateForFileFormat(NamedFormat::ORSONexus, false); }

  void testCustomOptionsDisabledForORSONexusFormat() {
    checkCustomOptionsStateForFileFormat(NamedFormat::ORSONexus, false);
  }

  void testQResolutionEnabledForORSONexusFormat() { checkQResolutionStateForFileFormat(NamedFormat::ORSONexus, true); }

  void testAdditionalColumnsEnabledForORSONexusFormat() {
    checkAdditionalColumnsStateForFileFormat(NamedFormat::ORSONexus, true);
  }

  void testSaveToSingleFileEnabledWithAutosaveForORSONexusFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ORSONexus, true, true);
  }

  void testSaveToSingleFileDisabledWithNoAutosaveForORSONexusFormat() {
    checkSaveToSingleFileStateForFileFormat(NamedFormat::ORSONexus, false, false);
  }

private:
  SavePresenter makePresenter() {
    auto FileSaver = std::make_unique<NiceMock<MockFileSaver>>();
    m_fileSaver = FileSaver.get();
    auto presenter = SavePresenter(&m_view, std::move(FileSaver));
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_fileSaver));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
    AnalysisDataService::Instance().clear();
  }

  Workspace2D_sptr createWorkspace(const std::string &name) {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(name, ws);
    return ws;
  }

  void createTableWorkspace(const std::string &name) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }

  std::vector<std::string> createWorkspaces(std::vector<std::string> const &workspaceNames = {"test1", "test2"}) {
    for (auto name : workspaceNames) {
      createWorkspace(name);
    }
    return workspaceNames;
  }

  void createWorkspaceGroup(const std::string &groupName, const std::vector<std::string> &workspaceNames) {
    AnalysisDataService::Instance().add(groupName, std::make_shared<WorkspaceGroup>());
    createWorkspaces(workspaceNames);
    for (auto name : workspaceNames)
      AnalysisDataService::Instance().addToGroup(groupName, name);
  }

  /* Add some dummy workspaces to the ADS with the given names and a log value
   * Theta */
  std::vector<std::string> createWorkspacesWithThetaLog(std::vector<std::string> const &workspaceNames = {"test1",
                                                                                                          "test2"}) {
    for (auto name : workspaceNames) {
      auto workspace = createWorkspace(name);
      workspace->mutableRun().addProperty("Theta", 0.5, true);
    }
    return workspaceNames;
  }

  /* Set the presenter up so that autosave is enabled */
  void enableAutosave(SavePresenter &presenter) {
    expectGetValidSaveDirectory();
    presenter.notifyAutosaveEnabled();
  }

  /* Set the presenter up so that autosave is disabled */
  void disableAutosave(SavePresenter &presenter) { presenter.notifyAutosaveDisabled(); }

  void expectSetWorkspaceListFromADS(std::vector<std::string> const &workspaceNames) {
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(workspaceNames)).Times(1);
  }

  void expectGetValidSaveDirectory() {
    EXPECT_CALL(m_view, getSavePath()).Times(1).WillOnce(Return(m_savePath));
    EXPECT_CALL(*m_fileSaver, isValidSaveDirectory(m_savePath)).Times(1).WillOnce(Return(true));
  }

  void expectGetInvalidSaveDirectory() {
    EXPECT_CALL(m_view, getSavePath()).Times(1).WillOnce(Return(m_savePath));
    EXPECT_CALL(*m_fileSaver, isValidSaveDirectory(m_savePath)).Times(1).WillOnce(Return(false));
  }

  void expectGetSaveParametersFromView(const bool saveToSingleFile, const bool isAutoSave) {
    EXPECT_CALL(m_view, getFileFormatIndex()).Times(1).WillOnce(Return(static_cast<int>(m_fileFormat)));
    EXPECT_CALL(m_view, getPrefix()).Times(1).WillOnce(Return(m_prefix));
    EXPECT_CALL(m_view, getHeaderCheck()).Times(1).WillOnce(Return(m_includeHeader));
    EXPECT_CALL(m_view, getSeparator()).Times(1).WillOnce(Return(m_separator));
    EXPECT_CALL(m_view, getQResolutionCheck()).Times(1).WillOnce(Return(m_includeQResolution));
    EXPECT_CALL(m_view, getAdditionalColumnsCheck()).Times(1).WillOnce(Return(m_includeAdditionalColumns));
    if (isAutoSave) {
      EXPECT_CALL(m_view, getSaveToSingleFileCheck()).Times(1).WillOnce(Return(saveToSingleFile));
    } else {
      EXPECT_CALL(m_view, getSaveToSingleFileCheck()).Times(0);
    }
  }

  void expectSaveWorkspaces(const std::vector<std::string> &workspaceNames,
                            const std::vector<std::string> &logs = std::vector<std::string>{}) {
    EXPECT_CALL(m_view, getSelectedParameters()).Times(1).WillOnce(Return(logs));
    expectGetValidSaveDirectory();
    expectGetSaveParametersFromView(false, false);
    auto fileFormatOptions = FileFormatOptions(m_fileFormat, m_prefix, m_includeHeader, m_separator,
                                               m_includeQResolution, m_includeAdditionalColumns, false);
    EXPECT_CALL(*m_fileSaver, save(m_savePath, workspaceNames, logs, fileFormatOptions)).Times(1);
  }

  void expectSaveWorkspacesNoLogs(const std::vector<std::string> &workspaceNames, const bool isSingleFileRequested,
                                  const bool isAutoSave, const bool expectedSingleFileOption) {
    expectGetValidSaveDirectory();
    expectGetSaveParametersFromView(isSingleFileRequested, isAutoSave);
    auto fileFormatOptions =
        FileFormatOptions(m_fileFormat, m_prefix, m_includeHeader, m_separator, m_includeQResolution,
                          m_includeAdditionalColumns, expectedSingleFileOption);
    EXPECT_CALL(*m_fileSaver, save(m_savePath, workspaceNames, _, fileFormatOptions)).Times(1);
  }

  void expectProcessing() { EXPECT_CALL(m_mainPresenter, isProcessing()).Times(1).WillOnce(Return(true)); }

  void expectAutoreducing() { EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(true)); }

  void expectNotProcessingOrAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isProcessing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_mainPresenter, isAutoreducing()).Times(1).WillOnce(Return(false));
  }

  void expectFileFormatAndLocationControlsEnabled() {
    EXPECT_CALL(m_view, enableFileFormatControls()).Times(1);
    EXPECT_CALL(m_view, enableLocationControls()).Times(1);
  }

  void expectFileFormatAndLocationControlsDisabled() {
    EXPECT_CALL(m_view, disableFileFormatControls()).Times(1);
    EXPECT_CALL(m_view, disableLocationControls()).Times(1);
  }

  void expectFileFormat(const NamedFormat fileFormat) {
    EXPECT_CALL(m_view, getFileFormatIndex()).Times(AtLeast(1)).WillOnce(Return(static_cast<int>(fileFormat)));
  }

  void expectHeaderOptionEnabled() { EXPECT_CALL(m_view, getHeaderCheck()).Times(AtLeast(1)).WillOnce(Return(true)); }

  void expectHeaderOptionDisabled() { EXPECT_CALL(m_view, getHeaderCheck()).Times(AtLeast(1)).WillOnce(Return(false)); }

  void expectLogListEnabled() { EXPECT_CALL(m_view, enableLogList()).Times(1); }

  void expectLogListDisabled() { EXPECT_CALL(m_view, disableLogList()).Times(1); }

  void expectQResolutionEnabled() { EXPECT_CALL(m_view, enableQResolutionCheckBox()).Times(1); }

  void expectQResolutionDisabled() { EXPECT_CALL(m_view, disableQResolutionCheckBox()).Times(1); }

  void expectCustomOptionsEnabled() {
    EXPECT_CALL(m_view, enableHeaderCheckBox()).Times(1);
    EXPECT_CALL(m_view, enableSeparatorButtonGroup()).Times(1);
  }

  void expectCustomOptionsDisabled() {
    EXPECT_CALL(m_view, disableHeaderCheckBox()).Times(1);
    EXPECT_CALL(m_view, disableSeparatorButtonGroup()).Times(1);
  }

  void expectAdditionalColumnsEnabled() { EXPECT_CALL(m_view, enableAdditionalColumnsCheckBox()).Times(1); }

  void expectAdditionalColumnsDisabled() { EXPECT_CALL(m_view, disableAdditionalColumnsCheckBox()).Times(1); }

  void expectSaveToSingleFileEnabled() { EXPECT_CALL(m_view, enableSaveToSingleFileCheckBox()).Times(1); }

  void expectSaveToSingleFileDisabled() { EXPECT_CALL(m_view, disableSaveToSingleFileCheckBox()).Times(1); }

  void checkQResolutionStateForFileFormat(const NamedFormat format, const bool isEnabled) {
    auto presenter = makePresenter();
    expectFileFormat(format);
    if (isEnabled) {
      expectQResolutionEnabled();
    } else {
      expectQResolutionDisabled();
    }
    presenter.notifySettingsChanged();
  }

  void checkLogListStateForFileFormat(const NamedFormat format, const bool isEnabled) {
    auto presenter = makePresenter();
    expectFileFormat(format);
    if (isEnabled) {
      expectLogListEnabled();
    } else {
      expectLogListDisabled();
    }
    presenter.notifySettingsChanged();
  }

  void checkCustomOptionsStateForFileFormat(const NamedFormat format, const bool isEnabled) {
    auto presenter = makePresenter();
    expectFileFormat(format);
    if (isEnabled) {
      expectCustomOptionsEnabled();
    } else {
      expectCustomOptionsDisabled();
    }
    presenter.notifySettingsChanged();
  }

  void checkAdditionalColumnsStateForFileFormat(const NamedFormat format, const bool isEnabled) {
    auto presenter = makePresenter();
    expectFileFormat(format);
    if (isEnabled) {
      expectAdditionalColumnsEnabled();
    } else {
      expectAdditionalColumnsDisabled();
    }
    presenter.notifySettingsChanged();
  }

  void checkSaveToSingleFileStateForFileFormat(const NamedFormat format, const bool isAutoSaveEnabled,
                                               const bool isEnabled) {
    auto presenter = makePresenter();
    if (isAutoSaveEnabled) {
      enableAutosave(presenter);
    }
    expectFileFormat(format);
    if (isEnabled) {
      expectSaveToSingleFileEnabled();
    } else {
      expectSaveToSingleFileDisabled();
    }
    presenter.notifySettingsChanged();
  }

  void checkNotifyAutosaveEnabledForFormat(const NamedFormat format, const bool isSingleFileEnabled) {
    auto presenter = makePresenter();
    expectGetValidSaveDirectory();
    EXPECT_CALL(m_view, enableSaveIndividualRowsCheckbox()).Times(1);
    expectFileFormat(format);
    if (isSingleFileEnabled) {
      expectSaveToSingleFileEnabled();
    } else {
      EXPECT_CALL(m_view, enableSaveToSingleFileCheckBox()).Times(0);
    }
    presenter.notifyAutosaveEnabled();
  }

  void runSaveWorkspacesTest(const bool isSingleFileRequested, const bool isAutoSave,
                             const bool expectedSingleFileOption) {
    auto presenter = makePresenter();
    auto const inputWorkspaces = std::vector<std::string>{"test1", "test2"};
    createWorkspaces(inputWorkspaces);
    expectSaveWorkspacesNoLogs(inputWorkspaces, isSingleFileRequested, isAutoSave, expectedSingleFileOption);
    presenter.saveWorkspaces(inputWorkspaces, isAutoSave);
  }

  NiceMock<MockSaveView> m_view;
  NiceMock<MockBatchPresenter> m_mainPresenter;
  NiceMock<MockFileSaver> *m_fileSaver;
  std::string m_savePath;
  // file format options for ascii saver
  NamedFormat m_fileFormat;
  std::string m_prefix;
  bool m_includeHeader;
  std::string m_separator;
  bool m_includeQResolution;
  bool m_includeAdditionalColumns;
};
