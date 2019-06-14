// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflSaveTabPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ConfigService.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

using Mantid::DataObjects::Workspace2D_sptr;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflSaveTabPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflSaveTabPresenterTest *createSuite() {
    return new ReflSaveTabPresenterTest();
  }
  static void destroySuite(ReflSaveTabPresenterTest *suite) { delete suite; }

  ReflSaveTabPresenterTest() { FrameworkManager::Instance(); }

  bool verifyAndClearMocks() {
    auto metViewExpections = Mock::VerifyAndClearExpectations(m_mockViewPtr);
    TS_ASSERT(metViewExpections);
    auto metSaverExpectations =
        Mock::VerifyAndClearExpectations(m_mockSaverPtr);
    TS_ASSERT(metSaverExpectations);
    return metViewExpections && metSaverExpectations;
  }

  void testPopulateWorkspaceList() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    std::vector<std::string> wsNames = {"ws1", "ws2", "ws3"};
    createWS(wsNames[0]);
    createWS(wsNames[1]);
    createWS(wsNames[2]);
    createTableWS("tableWS");

    // Group workspaces 1 and 2 together
    IAlgorithm_sptr groupAlg =
        AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->setProperty("InputWorkspaces", {"ws1", "ws2"});
    groupAlg->setProperty("OutputWorkspace", "groupWs");
    groupAlg->execute();

    EXPECT_CALL(*m_mockViewPtr, clearWorkspaceList()).Times(Exactly(1));
    // Workspaces 'groupWs' and 'tableWS' should not be included in the
    // workspace list
    EXPECT_CALL(*m_mockViewPtr, setWorkspaceList(wsNames)).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    AnalysisDataService::Instance().clear();

    TS_ASSERT(verifyAndClearMocks());
  }

  void testDisablesAutosaveControlsWhenProcessing() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    EXPECT_CALL(*m_mockViewPtr, disableAutosaveControls());
    presenter.onAnyReductionResumed();

    TS_ASSERT(verifyAndClearMocks());
  }

  void expectHasValidSaveDirectory(MockReflAsciiSaver &m_mockSaver) {
    ON_CALL(m_mockSaver, isValidSaveDirectory(_)).WillByDefault(Return(true));
  }

  void testDisablesFileFormatControlsWhenProcessingAndAutosaveEnabled() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    expectHasValidSaveDirectory(*m_mockSaverPtr);
    presenter.notify(IReflSaveTabPresenter::Flag::autosaveEnabled);

    EXPECT_CALL(*m_mockViewPtr, disableFileFormatAndLocationControls());
    presenter.onAnyReductionResumed();

    TS_ASSERT(verifyAndClearMocks());
  }

  void testEnablesFileFormatControlsWhenProcessingFinishedAndAutosaveEnabled() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    expectHasValidSaveDirectory(*m_mockSaverPtr);
    presenter.notify(IReflSaveTabPresenter::Flag::autosaveEnabled);
    presenter.onAnyReductionResumed();

    EXPECT_CALL(*m_mockViewPtr, enableFileFormatAndLocationControls());
    presenter.onAnyReductionPaused();

    TS_ASSERT(verifyAndClearMocks());
  }

  void testEnablesAutosaveControlsWhenProcessingFinished() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    expectHasValidSaveDirectory(*m_mockSaverPtr);
    presenter.notify(IReflSaveTabPresenter::Flag::autosaveEnabled);

    presenter.onAnyReductionResumed();
    EXPECT_CALL(*m_mockViewPtr, enableFileFormatAndLocationControls());
    presenter.onAnyReductionPaused();

    TS_ASSERT(verifyAndClearMocks());
  }

  void testRefreshWorkspaceList() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    createWS("ws1");

    EXPECT_CALL(*m_mockViewPtr, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(*m_mockViewPtr,
                setWorkspaceList(std::vector<std::string>{"ws1"}))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockViewPtr,
                setWorkspaceList(std::vector<std::string>{"ws1", "ws2"}))
        .Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    createWS("ws2");
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    AnalysisDataService::Instance().clear();

    TS_ASSERT(verifyAndClearMocks());
  }

  void testFilterWorkspaceNoRegex() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    createWS("anotherWs");
    createWS("different");
    createWS("someWsName");

    EXPECT_CALL(*m_mockViewPtr, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(*m_mockViewPtr, setWorkspaceList(std::vector<std::string>{
                                    "anotherWs", "different", "someWsName"}))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockViewPtr, getFilter())
        .Times(Exactly(1))
        .WillOnce(Return("Ws"));
    EXPECT_CALL(*m_mockViewPtr, getRegexCheck())
        .Times(Exactly(1))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_mockViewPtr, setWorkspaceList(std::vector<std::string>{
                                    "anotherWs", "someWsName"}))
        .Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    presenter.notify(IReflSaveTabPresenter::filterWorkspaceListFlag);
    AnalysisDataService::Instance().clear();

    TS_ASSERT(verifyAndClearMocks());
  }

  void testFilterWorkspaceWithRegex() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    createWS("_42");
    createWS("apple_113");
    createWS("grape_");
    createWS("pear_cut");

    EXPECT_CALL(*m_mockViewPtr, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(*m_mockViewPtr, setWorkspaceList(std::vector<std::string>{
                                    "_42", "apple_113", "grape_", "pear_cut"}))
        .Times(Exactly(1));
    EXPECT_CALL(*m_mockViewPtr, getFilter())
        .Times(Exactly(1))
        .WillOnce(Return("[a-zA-Z]*_[0-9]+"));
    EXPECT_CALL(*m_mockViewPtr, getRegexCheck())
        .Times(Exactly(1))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockViewPtr,
                setWorkspaceList(std::vector<std::string>{"_42", "apple_113"}))
        .Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    presenter.notify(IReflSaveTabPresenter::filterWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(verifyAndClearMocks());
  }

  void testPopulateParametersList() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    createWS("ws1");
    std::vector<std::string> logs;
    const auto &properties = AnalysisDataService::Instance()
                                 .retrieveWS<MatrixWorkspace>("ws1")
                                 ->run()
                                 .getProperties();
    for (auto it = properties.begin(); it != properties.end(); it++) {
      logs.push_back((*it)->name());
    }

    EXPECT_CALL(*m_mockViewPtr, clearParametersList()).Times(Exactly(1));
    EXPECT_CALL(*m_mockViewPtr, getCurrentWorkspaceName())
        .Times(Exactly(1))
        .WillOnce(Return("ws1"));
    EXPECT_CALL(*m_mockViewPtr, setParametersList(logs)).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::workspaceParamsFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(verifyAndClearMocks());
  }

  void testSaveWorkspaces() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    std::vector<std::string> wsNames = {"ws1", "ws2", "ws3"};
    createWS(wsNames[0]);
    createWS(wsNames[1]);
    createWS(wsNames[2]);

    EXPECT_CALL(*m_mockViewPtr, getSavePath()).Times(AtLeast(1));
    EXPECT_CALL(*m_mockViewPtr, getTitleCheck())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockViewPtr, getSelectedParameters())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::vector<std::string>()));
    EXPECT_CALL(*m_mockViewPtr, getQResolutionCheck())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockViewPtr, getSeparator())
        .Times(AtLeast(1))
        .WillRepeatedly(Return("comma"));
    EXPECT_CALL(*m_mockViewPtr, getPrefix())
        .Times(Exactly(1))
        .WillOnce(Return(""));
    EXPECT_CALL(*m_mockViewPtr, getFileFormatIndex())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockViewPtr, getSelectedWorkspaces())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(wsNames));

    EXPECT_CALL(*m_mockSaverPtr, isValidSaveDirectory(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockSaverPtr, save(_, _, _, _)).Times(AtLeast(1));

    presenter.notify(IReflSaveTabPresenter::saveWorkspacesFlag);

    AnalysisDataService::Instance().clear();
    TS_ASSERT(verifyAndClearMocks());
  }

  void testSuggestSaveDir() {
    ReflSaveTabPresenter presenter(std::move(m_mockSaver),
                                   std::move(m_mockView));

    std::string saveDir = Mantid::Kernel::ConfigService::Instance().getString(
        "defaultsave.directory");

    EXPECT_CALL(*m_mockViewPtr, setSavePath(saveDir));
    presenter.notify(IReflSaveTabPresenter::suggestSaveDirFlag);
    TS_ASSERT(verifyAndClearMocks());
  }

private:
  std::unique_ptr<NiceMock<MockReflAsciiSaver>> m_mockSaver;
  NiceMock<MockReflAsciiSaver> *m_mockSaverPtr;
  std::unique_ptr<MockSaveTabView> m_mockView;
  MockSaveTabView *m_mockViewPtr;

  void setUp() override {
    m_mockSaver = std::make_unique<NiceMock<MockReflAsciiSaver>>();
    m_mockSaverPtr = m_mockSaver.get();
    m_mockView = std::make_unique<MockSaveTabView>();
    m_mockViewPtr = m_mockView.get();
  }

  void createWS(std::string name) {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }

  void createTableWS(std::string name) {
    ITableWorkspace_sptr ws =
        WorkspaceFactory::Instance().createTable("TableWorkspace");
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H */
