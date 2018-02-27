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

  void test_instrument_name() {
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;
    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);
    ReflSettingsTabPresenter presenter(settingsPresenters);

    EXPECT_CALL(presenter_1, setInstrumentName("INSTRUMENT_NAME")).Times(1);
    EXPECT_CALL(presenter_2, setInstrumentName("INSTRUMENT_NAME")).Times(1);
    presenter.setInstrumentName("INSTRUMENT_NAME");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_transmission_runs() {
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

    EXPECT_CALL(presenter_0, getTransmissionRuns()).Times(1);
    EXPECT_CALL(presenter_1, getTransmissionRuns()).Times(0);
    EXPECT_CALL(presenter_2, getTransmissionRuns()).Times(0);
    presenter.getTransmissionRuns(0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getTransmissionRuns()).Times(0);
    EXPECT_CALL(presenter_1, getTransmissionRuns()).Times(1);
    EXPECT_CALL(presenter_2, getTransmissionRuns()).Times(0);
    presenter.getTransmissionRuns(1);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));

    EXPECT_CALL(presenter_0, getTransmissionRuns()).Times(0);
    EXPECT_CALL(presenter_1, getTransmissionRuns()).Times(0);
    EXPECT_CALL(presenter_2, getTransmissionRuns()).Times(1);
    presenter.getTransmissionRuns(2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_0));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter_2));
  }

  void test_transmission_options() {
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

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
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

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
    MockSettingsPresenter presenter_0;
    MockSettingsPresenter presenter_1;
    MockSettingsPresenter presenter_2;

    std::vector<IReflSettingsPresenter *> settingsPresenters;
    settingsPresenters.push_back(&presenter_0);
    settingsPresenters.push_back(&presenter_1);
    settingsPresenters.push_back(&presenter_2);

    ReflSettingsTabPresenter presenter(settingsPresenters);

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
