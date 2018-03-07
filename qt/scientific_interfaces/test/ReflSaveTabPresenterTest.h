#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "../ISISReflectometry/ReflSaveTabPresenter.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/make_unique.h"
#include "MantidAPI/Run.h"
#include "ReflMockObjects.h"
#include "Poco/File.h"
#include "Poco/Path.h"

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

  void testPopulateWorkspaceList() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

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

    EXPECT_CALL(mockViewRef, clearWorkspaceList()).Times(Exactly(1));
    // Workspaces 'groupWs' and 'tableWS' should not be included in the
    // workspace list
    EXPECT_CALL(mockViewRef, setWorkspaceList(wsNames)).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testDisablesAutosaveControlsWhenProcessing() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    EXPECT_CALL(mockViewRef, disableAutosaveControls());
    presenter.onAnyReductionResumed();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void expectHasValidSaveDirectory(MockReflAsciiSaver &mockSaver) {
    ON_CALL(mockSaver, isValidSaveDirectory(_)).WillByDefault(Return(true));
  }

  void testDisablesFileFormatControlsWhenProcessingAndAutosaveEnabled() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    auto &mockSaverRef = *mockSaver;
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    expectHasValidSaveDirectory(mockSaverRef);
    presenter.notify(IReflSaveTabPresenter::Flag::autosaveEnabled);

    EXPECT_CALL(mockViewRef, disableFileFormatAndLocationControls());
    presenter.onAnyReductionResumed();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testEnablesFileFormatControlsWhenProcessingFinishedAndAutosaveEnabled() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    auto &mockSaverRef = *mockSaver;
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    expectHasValidSaveDirectory(mockSaverRef);
    presenter.notify(IReflSaveTabPresenter::Flag::autosaveEnabled);
    presenter.onAnyReductionResumed();

    EXPECT_CALL(mockViewRef, enableFileFormatAndLocationControls());
    presenter.onAnyReductionPaused();
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testEnablesAutosaveControlsWhenProcessingFinished() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    auto &mockSaverRef = *mockSaver;
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    expectHasValidSaveDirectory(mockSaverRef);
    presenter.notify(IReflSaveTabPresenter::Flag::autosaveEnabled);

    presenter.onAnyReductionResumed();
    EXPECT_CALL(mockViewRef, enableFileFormatAndLocationControls());
    presenter.onAnyReductionPaused();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testRefreshWorkspaceList() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    createWS("ws1");

    EXPECT_CALL(mockViewRef, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(mockViewRef, setWorkspaceList(std::vector<std::string>{"ws1"}))
        .Times(Exactly(1));
    EXPECT_CALL(mockViewRef,
                setWorkspaceList(std::vector<std::string>{"ws1", "ws2"}))
        .Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    createWS("ws2");
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testFilterWorkspaceNoRegex() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    createWS("anotherWs");
    createWS("different");
    createWS("someWsName");

    EXPECT_CALL(mockViewRef, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(mockViewRef, setWorkspaceList(std::vector<std::string>{
                                 "anotherWs", "different", "someWsName"}))
        .Times(Exactly(1));
    EXPECT_CALL(mockViewRef, getFilter())
        .Times(Exactly(1))
        .WillOnce(Return("Ws"));
    EXPECT_CALL(mockViewRef, getRegexCheck())
        .Times(Exactly(1))
        .WillOnce(Return(false));
    EXPECT_CALL(mockViewRef, setWorkspaceList(std::vector<std::string>{
                                 "anotherWs", "someWsName"})).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    presenter.notify(IReflSaveTabPresenter::filterWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testFilterWorkspaceWithRegex() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    createWS("_42");
    createWS("apple_113");
    createWS("grape_");
    createWS("pear_cut");

    EXPECT_CALL(mockViewRef, clearWorkspaceList()).Times(Exactly(2));
    EXPECT_CALL(mockViewRef, setWorkspaceList(std::vector<std::string>{
                                 "_42", "apple_113", "grape_", "pear_cut"}))
        .Times(Exactly(1));
    EXPECT_CALL(mockViewRef, getFilter())
        .Times(Exactly(1))
        .WillOnce(Return("[a-zA-Z]*_[0-9]+"));
    EXPECT_CALL(mockViewRef, getRegexCheck())
        .Times(Exactly(1))
        .WillOnce(Return(true));
    EXPECT_CALL(mockViewRef,
                setWorkspaceList(std::vector<std::string>{"_42", "apple_113"}))
        .Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
    presenter.notify(IReflSaveTabPresenter::filterWorkspaceListFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testPopulateParametersList() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    createWS("ws1");
    std::vector<std::string> logs;
    const auto &properties = AnalysisDataService::Instance()
                                 .retrieveWS<MatrixWorkspace>("ws1")
                                 ->run()
                                 .getProperties();
    for (auto it = properties.begin(); it != properties.end(); it++) {
      logs.push_back((*it)->name());
    }

    EXPECT_CALL(mockViewRef, clearParametersList()).Times(Exactly(1));
    EXPECT_CALL(mockViewRef, getCurrentWorkspaceName())
        .Times(Exactly(1))
        .WillOnce(Return("ws1"));
    EXPECT_CALL(mockViewRef, setParametersList(logs)).Times(Exactly(1));
    presenter.notify(IReflSaveTabPresenter::workspaceParamsFlag);
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockView.get()));
  }

  void testSaveWorkspaces() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver = ::Mantid::Kernel::make_unique<MockReflAsciiSaver>();
    auto &mockSaverRef = *mockSaver;
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    std::vector<std::string> wsNames = {"ws1", "ws2", "ws3"};
    createWS(wsNames[0]);
    createWS(wsNames[1]);
    createWS(wsNames[2]);

    EXPECT_CALL(mockViewRef, getSavePath()).Times(AtLeast(1));
    EXPECT_CALL(mockViewRef, getTitleCheck())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockViewRef, getSelectedParameters())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::vector<std::string>()));
    EXPECT_CALL(mockViewRef, getQResolutionCheck())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockViewRef, getSeparator())
        .Times(AtLeast(1))
        .WillRepeatedly(Return("comma"));
    EXPECT_CALL(mockViewRef, getPrefix())
        .Times(Exactly(1))
        .WillOnce(Return(""));
    EXPECT_CALL(mockViewRef, getFileFormatIndex())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(mockViewRef, getSelectedWorkspaces())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(wsNames));

    EXPECT_CALL(mockSaverRef, isValidSaveDirectory(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockSaverRef, save(_, _, _, _)).Times(AtLeast(1));

    presenter.notify(IReflSaveTabPresenter::saveWorkspacesFlag);

    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockViewRef));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSaverRef));
  }

  void testSuggestSaveDir() {
    auto mockView = ::Mantid::Kernel::make_unique<MockSaveTabView>();
    auto &mockViewRef = *mockView;
    auto mockSaver =
        ::Mantid::Kernel::make_unique<NiceMock<MockReflAsciiSaver>>();
    auto &mockSaverRef = *mockView;
    ReflSaveTabPresenter presenter(std::move(mockSaver), std::move(mockView));

    std::string saveDir = Mantid::Kernel::ConfigService::Instance().getString(
        "defaultsave.directory");

    EXPECT_CALL(mockViewRef, setSavePath(saveDir));
    presenter.notify(IReflSaveTabPresenter::suggestSaveDirFlag);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockViewRef));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSaverRef));
  }

private:
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
