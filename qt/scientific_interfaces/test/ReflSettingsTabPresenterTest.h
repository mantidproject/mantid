#ifndef MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/algorithm/string.hpp>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "../ISISReflectometry/ReflSettingsTabPresenter.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflSettingsTabPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflSettingsTabPresenterTest *createSuite() {
    return new ReflSettingsTabPresenterTest();
  }
  static void destroySuite(ReflSettingsTabPresenterTest *suite) {
    delete suite;
  }

  ReflSettingsTabPresenterTest() { FrameworkManager::Instance(); }

  void test_set_instrument_name() {
    // Test setting the instrument name

    // Set up settings presenters for 2 groups
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;
    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflSettingsTabPresenter presenter(settingsPresenters);

    // Should set the instrument name on the settings presenters for
    // all groups
    EXPECT_CALL(presenter_1, setInstrumentName("INSTRUMENT_NAME")).Times(1);
    EXPECT_CALL(presenter_2, setInstrumentName("INSTRUMENT_NAME")).Times(1);
    presenter.setInstrumentName("INSTRUMENT_NAME");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_check_transmission_runs_per_angle() {
    // Test checking whether transmission runs are available per angle

    // Set up settings presenters for 3 groups
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

    // Should only call though to the settings presenter for
    // the specified group
    EXPECT_CALL(presenter_0, hasPerAngleOptions()).Times(1);
    EXPECT_CALL(presenter_1, hasPerAngleOptions()).Times(0);
    EXPECT_CALL(presenter_2, hasPerAngleOptions()).Times(0);
    presenter.hasPerAngleOptions(0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, hasPerAngleOptions()).Times(0);
    EXPECT_CALL(presenter_1, hasPerAngleOptions()).Times(1);
    EXPECT_CALL(presenter_2, hasPerAngleOptions()).Times(0);
    presenter.hasPerAngleOptions(1);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, hasPerAngleOptions()).Times(0);
    EXPECT_CALL(presenter_1, hasPerAngleOptions()).Times(0);
    EXPECT_CALL(presenter_2, hasPerAngleOptions()).Times(1);
    presenter.hasPerAngleOptions(2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_get_transmission_runs_for_angle() {
    // Test getting the transmission runs for a particular angle

    // Set up settings presenters for 3 groups
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);
    const double angle = 0.5;

    // Should only call though to the settings presenter for
    // the specified group
    EXPECT_CALL(presenter_0, getOptionsForAngle(angle))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(presenter_1, getOptionsForAngle(angle)).Times(0);
    EXPECT_CALL(presenter_2, getOptionsForAngle(angle)).Times(0);
    presenter.getOptionsForAngle(0, angle);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getOptionsForAngle(angle)).Times(0);
    EXPECT_CALL(presenter_1, getOptionsForAngle(angle))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(presenter_2, getOptionsForAngle(angle)).Times(0);
    presenter.getOptionsForAngle(1, angle);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getOptionsForAngle(angle)).Times(0);
    EXPECT_CALL(presenter_1, getOptionsForAngle(angle)).Times(0);
    EXPECT_CALL(presenter_2, getOptionsForAngle(angle))
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    presenter.getOptionsForAngle(2, angle);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_transmission_options() {
    // Test getting options for the preprocessing algorithm to
    // create the transmission workspace

    // Set up settings presenters for 3 groups
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

    // Should only call though to the settings presenter for
    // the specified group
    EXPECT_CALL(presenter_0, getTransmissionOptions())
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(presenter_1, getTransmissionOptions()).Times(0);
    EXPECT_CALL(presenter_2, getTransmissionOptions()).Times(0);
    presenter.getTransmissionOptions(0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getTransmissionOptions()).Times(0);
    EXPECT_CALL(presenter_1, getTransmissionOptions())
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(presenter_2, getTransmissionOptions()).Times(0);
    presenter.getTransmissionOptions(1);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getTransmissionOptions()).Times(0);
    EXPECT_CALL(presenter_1, getTransmissionOptions()).Times(0);
    EXPECT_CALL(presenter_2, getTransmissionOptions())
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    presenter.getTransmissionOptions(2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_reduction_options() {
    // Test getting options for the main reduction algorithm

    // Set up settings presenters for 3 groups
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

    // Should only call though to the settings presenter for
    // the specified group
    EXPECT_CALL(presenter_0, getReductionOptions())
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(presenter_1, getReductionOptions()).Times(0);
    EXPECT_CALL(presenter_2, getReductionOptions()).Times(0);
    presenter.getReductionOptions(0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getReductionOptions()).Times(0);
    EXPECT_CALL(presenter_1, getReductionOptions())
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    EXPECT_CALL(presenter_2, getReductionOptions()).Times(0);
    presenter.getReductionOptions(1);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getReductionOptions()).Times(0);
    EXPECT_CALL(presenter_1, getReductionOptions()).Times(0);
    EXPECT_CALL(presenter_2, getReductionOptions())
        .Times(1)
        .WillOnce(Return(OptionsQMap()));
    presenter.getReductionOptions(2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_stitch_options() {
    // Test getting options for the postprocessing algorithm
    // to stitch workspaces

    // Set up settings presenters for 3 groups
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

    // Should only call though to the settings presenter for
    // the specified group
    EXPECT_CALL(presenter_0, getStitchOptions()).Times(1);
    EXPECT_CALL(presenter_1, getStitchOptions()).Times(0);
    EXPECT_CALL(presenter_2, getStitchOptions()).Times(0);
    presenter.getStitchOptions(0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getStitchOptions()).Times(0);
    EXPECT_CALL(presenter_1, getStitchOptions()).Times(1);
    EXPECT_CALL(presenter_2, getStitchOptions()).Times(0);
    presenter.getStitchOptions(1);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getStitchOptions()).Times(0);
    EXPECT_CALL(presenter_1, getStitchOptions()).Times(0);
    EXPECT_CALL(presenter_2, getStitchOptions()).Times(1);
    presenter.getStitchOptions(2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_passes_message_to_child_presenters_when_reduction_paused() {
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflSettingsTabPresenter presenter(settingsPresenters);

    EXPECT_CALL(presenter_1, onReductionPaused());

    presenter.onReductionPaused(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_passes_message_to_child_presenters_when_reduction_resumed() {
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflSettingsTabPresenter presenter(settingsPresenters);

    EXPECT_CALL(presenter_1, onReductionResumed());

    presenter.onReductionResumed(1);

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSETTINGSTABPRESENTERTEST_H */
