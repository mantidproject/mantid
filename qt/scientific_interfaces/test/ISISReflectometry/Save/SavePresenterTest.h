// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_SAVEPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_SAVEPRESENTERTEST_H_

#include "../ReflMockObjects.h"
#include "GUI/Save/IAsciiSaver.h"
#include "GUI/Save/SavePresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MockSaveView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::MantidWidgets::DataProcessor;
using Mantid::API::AlgorithmManager;
using Mantid::API::AnalysisDataService;
using Mantid::DataObjects::Workspace2D_sptr;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class SavePresenterTest : public CxxTest::TestSuite {
public:
  static SavePresenterTest *createSuite() { return new SavePresenterTest(); }
  static void destroySuite(SavePresenterTest *suite) { delete suite; }

  SavePresenterTest()
      : m_view(), m_savePath("/foo/bar/"), m_fileFormat(NamedFormat::Custom),
        m_prefix("testoutput_"), m_includeTitle(true), m_separator(","),
        m_includeQResolution(true) {}

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testNotifyPopulateWorkspaceList() {
    auto presenter = makePresenter();
    auto workspaceNames = createWorkspaces();
    expectSetWorkspaceListFromADS(workspaceNames);
    presenter.notifyPopulateWorkspaceList();
    verifyAndClear();
  }

  void testUpdateWorkspaceList() {
    auto presenter = makePresenter();
    createWorkspace("ws1");
    expectSetWorkspaceListFromADS({"ws1"});
    presenter.notifyPopulateWorkspaceList();
    createWorkspace("ws2");
    expectSetWorkspaceListFromADS({"ws1", "ws2"});
    presenter.notifyPopulateWorkspaceList();
    verifyAndClear();
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
    verifyAndClear();
  }

  void testNotifyFilterWorkspaceList() {
    auto presenter = makePresenter();
    auto const filter = std::string("Ws");
    auto const inputWorkspaces =
        std::vector<std::string>{"someWsName", "different", "anotherWs"};
    auto const filteredWorkspaces =
        std::vector<std::string>{"anotherWs", "someWsName"};
    createWorkspaces(inputWorkspaces);
    EXPECT_CALL(m_view, getFilter()).Times(1).WillOnce(Return(filter));
    EXPECT_CALL(m_view, getRegexCheck()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(filteredWorkspaces)).Times(1);
    presenter.notifyFilterWorkspaceList();
    verifyAndClear();
  }

  void testNotifyFilterWorkspaceListByRegex() {
    auto presenter = makePresenter();
    auto const filter = std::string("[a-zA-Z]*_[0-9]+");
    auto const inputWorkspaces =
        std::vector<std::string>{"_42", "apple_113", "grape_", "pear_cut"};
    auto const filteredWorkspaces =
        std::vector<std::string>{"_42", "apple_113"};
    createWorkspaces(inputWorkspaces);
    EXPECT_CALL(m_view, getFilter()).Times(1).WillOnce(Return(filter));
    EXPECT_CALL(m_view, getRegexCheck()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(m_view, showFilterEditValid()).Times(1);
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(filteredWorkspaces)).Times(1);
    presenter.notifyFilterWorkspaceList();
    verifyAndClear();
  }

  void testNotifyFilterWorkspaceListWithInvalidRegex() {
    auto presenter = makePresenter();
    auto const filter = std::string("w[.*kspace");
    auto const inputWorkspaces = std::vector<std::string>{
        "first_test_workspace", "test_ws_2", "dummy_wkspace"};
    auto const filteredWorkspaces = std::vector<std::string>{};
    createWorkspaces(inputWorkspaces);
    EXPECT_CALL(m_view, getFilter()).Times(1).WillOnce(Return(filter));
    EXPECT_CALL(m_view, getRegexCheck()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(m_view, showFilterEditInvalid()).Times(1);
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(filteredWorkspaces)).Times(1);
    presenter.notifyFilterWorkspaceList();
    verifyAndClear();
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
    EXPECT_CALL(m_view, getCurrentWorkspaceName())
        .Times(1)
        .WillOnce(Return(currentWorkspace));
    EXPECT_CALL(m_view, setParametersList(expectedLogs)).Times(1);
    presenter.notifyPopulateParametersList();
    verifyAndClear();
  }

  void testNotifySaveSelectedWorkspacesWithLogs() {
    auto presenter = makePresenter();
    auto const inputWorkspaces =
        std::vector<std::string>{"test1", "test2", "test3", "test4"};
    createWorkspacesWithThetaLog(inputWorkspaces);
    auto const logs = std::vector<std::string>{"Theta"};
    auto selectedWorkspaces = std::vector<std::string>{"test2", "test4"};
    EXPECT_CALL(m_view, getSelectedWorkspaces())
        .Times(1)
        .WillOnce(Return(selectedWorkspaces));
    expectSaveWorkspaces(selectedWorkspaces, logs);
    presenter.notifySaveSelectedWorkspaces();
    verifyAndClear();
  }

  void testNotifySaveSelectedWorkspacesWhenNothingSelected() {
    auto presenter = makePresenter();
    auto emptyWorkspaceList = std::vector<std::string>{};
    EXPECT_CALL(m_view, getSelectedWorkspaces())
        .Times(1)
        .WillOnce(Return(emptyWorkspaceList));
    EXPECT_CALL(m_view, noWorkspacesSelected()).Times(1);
    presenter.notifySaveSelectedWorkspaces();
    verifyAndClear();
  }

  void testNotifySuggestSaveDir() {
    auto presenter = makePresenter();
    auto const path = Mantid::Kernel::ConfigService::Instance().getString(
        "defaultsave.directory");
    EXPECT_CALL(m_view, setSavePath(path)).Times(1);
    presenter.notifySuggestSaveDir();
    verifyAndClear();
  }

  void testNotifyAutosaveDisabled() {
    auto presenter = makePresenter();
    // There are no calls to the view
    presenter.notifyAutosaveDisabled();
    verifyAndClear();
  }

  void testNotifyAutosaveEnabled() {
    auto presenter = makePresenter();
    expectGetValidSaveDirectory();
    presenter.notifyAutosaveEnabled();
    verifyAndClear();
  }

  void testNotifyAutosaveEnabledWithInvalidPath() {
    auto presenter = makePresenter();
    expectGetInvalidSaveDirectory();
    EXPECT_CALL(m_view, disallowAutosave()).Times(1);
    EXPECT_CALL(m_view, errorInvalidSaveDirectory()).Times(1);
    presenter.notifyAutosaveEnabled();
    verifyAndClear();
  }

  void testNotifySavePathChangedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectGetValidSaveDirectory();
    presenter.notifySavePathChanged();
    verifyAndClear();
  }

  void testNotifySavePathChangedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    EXPECT_CALL(m_view, getSavePath()).Times(0);
    presenter.notifySavePathChanged();
    verifyAndClear();
  }

  void testNotifySavePathChangedWithInvalidPath() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectGetInvalidSaveDirectory();
    EXPECT_CALL(m_view, warnInvalidSaveDirectory()).Times(1);
    presenter.notifySavePathChanged();
    verifyAndClear();
  }

  void testControlsEnabledWhenReductionPaused() {
    auto presenter = makePresenter();
    auto workspaceNames = createWorkspaces();
    expectSetWorkspaceListFromADS(workspaceNames);
    expectNotProcessingOrAutoreducing();
    EXPECT_CALL(m_view, enableAutosaveControls()).Times(1);
    EXPECT_CALL(m_view, enableFileFormatAndLocationControls()).Times(1);
    presenter.reductionPaused();
    verifyAndClear();
  }

  void testAutosaveControlsDisabledWhenReductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectProcessing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.reductionResumed();
    verifyAndClear();
  }

  void testFileControlsDisabledWhenReductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectProcessing();
    EXPECT_CALL(m_view, disableFileFormatAndLocationControls()).Times(1);
    presenter.reductionResumed();
    verifyAndClear();
  }

  void testFileControlsEnabledWhenReductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectProcessing();
    EXPECT_CALL(m_view, enableFileFormatAndLocationControls()).Times(1);
    presenter.reductionResumed();
    verifyAndClear();
  }

  void testAutosaveControlsDisabledWhenReductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectProcessing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.reductionResumed();
    verifyAndClear();
  }

  void testAutosaveControlsDisabledWhenAutoreductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectAutoreducing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.autoreductionResumed();
    verifyAndClear();
  }

  void testFileControlsDisabledWhenAutoreductionResumedWithAutosaveOn() {
    auto presenter = makePresenter();
    enableAutosave(presenter);
    expectAutoreducing();
    EXPECT_CALL(m_view, disableFileFormatAndLocationControls()).Times(1);
    presenter.autoreductionResumed();
    verifyAndClear();
  }

  void testFileControlsEnabledWhenAutoreductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectAutoreducing();
    EXPECT_CALL(m_view, enableFileFormatAndLocationControls()).Times(1);
    presenter.autoreductionResumed();
    verifyAndClear();
  }

  void testAutosaveControlsDisabledWhenAutoreductionResumedWithAutosaveOff() {
    auto presenter = makePresenter();
    disableAutosave(presenter);
    expectAutoreducing();
    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    presenter.autoreductionResumed();
    verifyAndClear();
  }

  void testAutosaveDisabledNotifiesMainPresenter() {
    auto presenter = makePresenter();
    presenter.notifyAutosaveDisabled();
    verifyAndClear();
  }

  void testAutosaveEnabledNotifiesMainPresenter() {
    auto presenter = makePresenter();
    presenter.notifyAutosaveEnabled();
    verifyAndClear();
  }

private:
  SavePresenter makePresenter() {
    auto asciiSaver = std::make_unique<NiceMock<MockAsciiSaver>>();
    m_asciiSaver = asciiSaver.get();
    auto presenter = SavePresenter(&m_view, std::move(asciiSaver));
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_asciiSaver));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_mainPresenter));
    AnalysisDataService::Instance().clear();
  }

  Workspace2D_sptr createWorkspace(std::string name) {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(name, ws);
    return ws;
  }

  void createTableWorkspace(std::string name) {
    ITableWorkspace_sptr ws =
        WorkspaceFactory::Instance().createTable("TableWorkspace");
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }

  std::vector<std::string> createWorkspaces(
      std::vector<std::string> const &workspaceNames = {"test1", "test2"}) {
    for (auto name : workspaceNames) {
      createWorkspace(name);
    }
    return workspaceNames;
  }

  void createWorkspaceGroup(std::string groupName,
                            std::vector<std::string> workspaceNames) {
    AnalysisDataService::Instance().add(groupName,
                                        boost::make_shared<WorkspaceGroup>());
    createWorkspaces(workspaceNames);
    for (auto name : workspaceNames)
      AnalysisDataService::Instance().addToGroup(groupName, name);
  }

  /* Add some dummy workspaces to the ADS with the given names and a log value
   * Theta */
  std::vector<std::string> createWorkspacesWithThetaLog(
      std::vector<std::string> const &workspaceNames = {"test1", "test2"}) {
    for (auto name : workspaceNames) {
      auto workspace = createWorkspace(name);
      workspace->mutableRun().addProperty("Theta", 0.5, true);
    }
    return workspaceNames;
  }

  /* Set the presenter up so that autosave is enabled. This clears any
   * expectations caused by its own calls so do this before setting
   * expectations in the calling fuction */
  void enableAutosave(SavePresenter &presenter) {
    expectGetValidSaveDirectory();
    presenter.notifyAutosaveEnabled();
    verifyAndClear();
  }

  /* Set the presenter up so that autosave is disabled */
  void disableAutosave(SavePresenter &presenter) {
    presenter.notifyAutosaveDisabled();
  }

  void expectSetWorkspaceListFromADS(
      std::vector<std::string> const &workspaceNames) {
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(workspaceNames)).Times(1);
  }

  void expectGetValidSaveDirectory() {
    EXPECT_CALL(m_view, getSavePath()).Times(1).WillOnce(Return(m_savePath));
    EXPECT_CALL(*m_asciiSaver, isValidSaveDirectory(m_savePath))
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectGetInvalidSaveDirectory() {
    EXPECT_CALL(m_view, getSavePath()).Times(1).WillOnce(Return(m_savePath));
    EXPECT_CALL(*m_asciiSaver, isValidSaveDirectory(m_savePath))
        .Times(1)
        .WillOnce(Return(false));
  }

  void expectGetSaveParametersFromView() {
    EXPECT_CALL(m_view, getFileFormatIndex())
        .Times(1)
        .WillOnce(Return(static_cast<int>(m_fileFormat)));
    EXPECT_CALL(m_view, getPrefix()).Times(1).WillOnce(Return(m_prefix));
    EXPECT_CALL(m_view, getTitleCheck())
        .Times(1)
        .WillOnce(Return(m_includeTitle));
    EXPECT_CALL(m_view, getSeparator()).Times(1).WillOnce(Return(m_separator));
    EXPECT_CALL(m_view, getQResolutionCheck())
        .Times(1)
        .WillOnce(Return(m_includeQResolution));
  }

  void expectSaveWorkspaces(
      std::vector<std::string> workspaceNames,
      std::vector<std::string> logs = std::vector<std::string>{}) {
    EXPECT_CALL(m_view, getSelectedParameters())
        .Times(1)
        .WillOnce(Return(logs));
    expectGetValidSaveDirectory();
    expectGetSaveParametersFromView();
    auto fileFormatOptions =
        FileFormatOptions(m_fileFormat, m_prefix, m_includeTitle, m_separator,
                          m_includeQResolution);
    EXPECT_CALL(*m_asciiSaver,
                save(m_savePath, workspaceNames, logs, fileFormatOptions))
        .Times(1);
  }

  void expectProcessing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectNotProcessingOrAutoreducing() {
    EXPECT_CALL(m_mainPresenter, isProcessing())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(m_mainPresenter, isAutoreducing())
        .Times(1)
        .WillOnce(Return(false));
  }

  NiceMock<MockSaveView> m_view;
  NiceMock<MockBatchPresenter> m_mainPresenter;
  NiceMock<MockAsciiSaver> *m_asciiSaver;
  std::string m_savePath;
  // file format options for ascii saver
  NamedFormat m_fileFormat;
  std::string m_prefix;
  bool m_includeTitle;
  std::string m_separator;
  bool m_includeQResolution;
};
#endif // MANTID_CUSTOMINTERFACES_SAVEPRESENTERTEST_H_
