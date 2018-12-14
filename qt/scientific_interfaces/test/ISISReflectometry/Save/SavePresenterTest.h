// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_SAVEPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_SAVEPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Save/SavePresenter.h"
#include "../../ReflMockObjects.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MockSaveView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using Mantid::API::AnalysisDataService;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class SavePresenterTest : public CxxTest::TestSuite {
public:
  static SavePresenterTest *createSuite() { return new SavePresenterTest(); }
  static void destroySuite(SavePresenterTest *suite) { delete suite; }

  SavePresenterTest() : m_view() {}

  void testNotifyPopulateWorkspaceList() {
    auto presenter = makePresenter();
    expectSetWorkspaceListFromADS();
    presenter.notifyPopulateWorkspaceList();
    verifyAndClear();
  }

  void testNotifyFilterWorkspaceList() {}

  void testNotifyPopulateParametersList() {}

  void testNotifySaveSelectedWorkspaces() {}

  void testNotifySuggestSaveDir() {}

  void testNotifyAutosaveDisabled() {}

  void testNotifyAutosaveEnabled() {}

  void testNotifySavePathChanged() {}

  void testReductionPaused() {
    auto presenter = makePresenter();
    expectSetWorkspaceListFromADS();
    EXPECT_CALL(m_view, enableAutosaveControls()).Times(1);
    EXPECT_CALL(m_view, enableFileFormatAndLocationControls()).Times(1);
    presenter.onAnyReductionPaused();
    verifyAndClear();
  }

  void testEnableAutosave() {
    auto presenter = makePresenter();
    expectGetValidSaveDirectory();
    presenter.notifyAutosaveEnabled();
    verifyAndClear();
  }

  void testReductionResumedWhenAutosaveEnabled() {
    auto presenter = makePresenter();
    // Enable autosave
    expectGetValidSaveDirectory();
    presenter.notifyAutosaveEnabled();
    verifyAndClear();

    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    EXPECT_CALL(m_view, disableFileFormatAndLocationControls()).Times(1);
    presenter.onAnyReductionResumed();
    verifyAndClear();
  }

  void testReductionResumedWhenAutosaveDisabled() {
    auto presenter = makePresenter();
    // Disable autosave
    presenter.notifyAutosaveDisabled();

    EXPECT_CALL(m_view, disableAutosaveControls()).Times(1);
    EXPECT_CALL(m_view, disableFileFormatAndLocationControls()).Times(0);
    presenter.onAnyReductionResumed();
    verifyAndClear();
  }

private:
  SavePresenter makePresenter() {
    auto asciiSaver = Mantid::Kernel::make_unique<MockReflAsciiSaver>();
    m_asciiSaver = asciiSaver.get();
    auto presenter = SavePresenter(&m_view, std::move(asciiSaver));
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void expectSetWorkspaceListFromADS() {
    // Add some workspaces to the ADS
    auto const workspaceNames = std::vector<std::string>{"test1", "test2"};
    auto const dummyWS = Mantid::API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, 1, 1);
    for (auto name : workspaceNames)
      AnalysisDataService::Instance().addOrReplace(name, dummyWS);
    EXPECT_CALL(m_view, clearWorkspaceList()).Times(1);
    EXPECT_CALL(m_view, setWorkspaceList(workspaceNames)).Times(1);
  }

  void expectGetValidSaveDirectory() {
    auto directory = std::string("dummy");
    EXPECT_CALL(m_view, getSavePath()).Times(1).WillOnce(Return(directory));
    EXPECT_CALL(*m_asciiSaver, isValidSaveDirectory(directory))
        .Times(1)
        .WillOnce(Return(true));
  }

  MockSaveView m_view;
  MockReflAsciiSaver *m_asciiSaver;
};
#endif // MANTID_CUSTOMINTERFACES_SAVEPRESENTERTEST_H_
