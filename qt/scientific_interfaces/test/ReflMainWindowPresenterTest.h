// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../ISISReflectometry/ReflMainWindowPresenter.h"

#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainWindowPresenterTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMainWindowPresenterTest *createSuite() {
    return new ReflMainWindowPresenterTest();
  }
  static void destroySuite(ReflMainWindowPresenterTest *suite) { delete suite; }

  ReflMainWindowPresenterTest() {}

  void testGetTransmissionValues() {
    // Test getting transmission run values
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsTabPresenter;
    MockEventTabPresenter mockEventTabPresenter;
    MockSettingsTabPresenter mockSettingsTabPresenter;
    auto mockSaveTabPresenter = std::make_unique<MockSaveTabPresenter>();
    ReflMainWindowPresenter presenter(
        &mockView, &mockRunsTabPresenter, &mockEventTabPresenter,
        &mockSettingsTabPresenter, std::move(mockSaveTabPresenter));

    // Should call the settings tab to get the values
    double angle = 0.5;
    EXPECT_CALL(mockSettingsTabPresenter, getOptionsForAngle(0, angle))
        .Times(Exactly(1))
        .WillOnce(Return(OptionsQMap()));
    presenter.getOptionsForAngle(0, angle);

    EXPECT_CALL(mockSettingsTabPresenter, getOptionsForAngle(1, angle))
        .Times(Exactly(1))
        .WillOnce(Return(OptionsQMap()));
    presenter.getOptionsForAngle(1, angle);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSettingsTabPresenter));
  }

  void testGetTransmissionOptions() {
    // Test getting options for the preprocessing algorithm that creates
    // the transmission workspace
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockEventTabPresenter mockEventPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    auto mockSaveTabPresenter = std::make_unique<MockSaveTabPresenter>();
    ReflMainWindowPresenter presenter(
        &mockView, &mockRunsPresenter, &mockEventPresenter,
        &mockSettingsPresenter, std::move(mockSaveTabPresenter));

    // Should call the settings tab to get the options
    EXPECT_CALL(mockSettingsPresenter, getTransmissionOptions(0))
        .Times(Exactly(1))
        .WillOnce(Return(OptionsQMap()));
    presenter.getTransmissionOptions(0);

    EXPECT_CALL(mockSettingsPresenter, getTransmissionOptions(1))
        .Times(Exactly(1))
        .WillOnce(Return(OptionsQMap()));
    presenter.getTransmissionOptions(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSettingsPresenter));
  }

  void testGetReductionOptions() {
    // Test getting the options for the main reduction algorithm
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockEventTabPresenter mockEventPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    auto mockSaveTabPresenter = std::make_unique<MockSaveTabPresenter>();
    ReflMainWindowPresenter presenter(
        &mockView, &mockRunsPresenter, &mockEventPresenter,
        &mockSettingsPresenter, std::move(mockSaveTabPresenter));

    // Should call the settings tab to get the options
    EXPECT_CALL(mockSettingsPresenter, getReductionOptions(0))
        .Times(Exactly(1))
        .WillOnce(Return(OptionsQMap()));
    presenter.getReductionOptions(0);

    EXPECT_CALL(mockSettingsPresenter, getReductionOptions(1))
        .Times(Exactly(1))
        .WillOnce(Return(OptionsQMap()));
    presenter.getReductionOptions(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSettingsPresenter));
  }

  void testStitchOptions() {
    // Test getting the options for the post-processing algorithm for
    // stitching workspaces
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockEventTabPresenter mockEventPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    auto mockSaveTabPresenter = std::make_unique<MockSaveTabPresenter>();
    ReflMainWindowPresenter presenter(
        &mockView, &mockRunsPresenter, &mockEventPresenter,
        &mockSettingsPresenter, std::move(mockSaveTabPresenter));

    // Should call the settings tab to get the options
    EXPECT_CALL(mockSettingsPresenter, getStitchOptions(0)).Times(Exactly(1));
    presenter.getStitchOptions(0);

    EXPECT_CALL(mockSettingsPresenter, getStitchOptions(1)).Times(Exactly(1));
    presenter.getStitchOptions(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockSettingsPresenter));
  }

  void testGiveUserCritical() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockEventTabPresenter mockEventPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    auto mockSaveTabPresenter = std::make_unique<MockSaveTabPresenter>();
    ReflMainWindowPresenter presenter(
        &mockView, &mockRunsPresenter, &mockEventPresenter,
        &mockSettingsPresenter, std::move(mockSaveTabPresenter));

    EXPECT_CALL(mockView, giveUserCritical("Prompt", "Title"))
        .Times(Exactly(1));
    presenter.giveUserCritical("Prompt", "Title");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testGiveUserInfo() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockEventTabPresenter mockEventPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    auto mockSaveTabPresenter = std::make_unique<MockSaveTabPresenter>();
    ReflMainWindowPresenter presenter(
        &mockView, &mockRunsPresenter, &mockEventPresenter,
        &mockSettingsPresenter, std::move(mockSaveTabPresenter));

    EXPECT_CALL(mockView, giveUserInfo("Prompt", "Title")).Times(Exactly(1));
    presenter.giveUserInfo("Prompt", "Title");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }

  void testUserPythonCode() {
    MockMainWindowView mockView;
    MockRunsTabPresenter mockRunsPresenter;
    MockEventTabPresenter mockEventPresenter;
    MockSettingsTabPresenter mockSettingsPresenter;
    auto mockSaveTabPresenter = std::make_unique<MockSaveTabPresenter>();
    ReflMainWindowPresenter presenter(
        &mockView, &mockRunsPresenter, &mockEventPresenter,
        &mockSettingsPresenter, std::move(mockSaveTabPresenter));

    EXPECT_CALL(mockView, runPythonAlgorithm("Python code to run"))
        .Times(Exactly(1));
    presenter.runPythonAlgorithm("Python code to run");
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTERTEST_H */
