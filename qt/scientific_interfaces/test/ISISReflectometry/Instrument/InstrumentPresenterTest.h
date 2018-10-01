#ifndef MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Instrument/InstrumentPresenter.h"
#include "MockInstrumentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class InstrumentPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentPresenterTest *createSuite() {
    return new InstrumentPresenterTest();
  }
  static void destroySuite(InstrumentPresenterTest *suite) { delete suite; }

  InstrumentPresenterTest() : m_view() {}

  bool verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    return true;
  }

  Instrument makeModel() {
    auto wavelengthRange = RangeInLambda(0.0, 0.0);
    auto monitorCorrections = MonitorCorrections(
        0, true, RangeInLambda(0.0, 0.0), RangeInLambda(0.0, 0.0));
    auto detectorCorrections =
        DetectorCorrections(false, DetectorCorrectionType::VerticalShift);
    return Instrument(wavelengthRange, monitorCorrections, detectorCorrections);
  }

  InstrumentPresenter makePresenter() {
    return InstrumentPresenter(&m_view, makeModel());
  }

  void testSetWavelengthRange() {
    auto presenter = makePresenter();
    auto const range = RangeInLambda(1.5, 14);

    EXPECT_CALL(m_view, getLambdaMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getLambdaMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showLambdaRangeValid()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().wavelengthRange(), range);
    TS_ASSERT(presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testSetWavelengthRangeInvalid() {
    auto presenter = makePresenter();
    auto const range = RangeInLambda(7.5, 2);

    EXPECT_CALL(m_view, getLambdaMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getLambdaMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showLambdaRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().wavelengthRange(), range);
    TS_ASSERT(!presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testIntegratedMonitorsToggled() {
    auto presenter = makePresenter();
    auto const integrate = !presenter.instrument().integratedMonitors();

    EXPECT_CALL(m_view, getIntegrateMonitors()).WillOnce(Return(integrate));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().integratedMonitors(), integrate);
    TS_ASSERT(presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testSetMonitorIndex() {
    auto presenter = makePresenter();
    auto const monitorIndex = 3;

    EXPECT_CALL(m_view, getMonitorIndex()).WillOnce(Return(monitorIndex));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().monitorIndex(), monitorIndex);
    TS_ASSERT(presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testSetMonitorIntegralRange() {
    auto presenter = makePresenter();
    auto const range = RangeInLambda(3.4, 12.2);

    EXPECT_CALL(m_view, getMonitorIntegralMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorIntegralMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorIntegralRangeValid()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().monitorIntegralRange(), range);
    TS_ASSERT(presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testSetMonitorIntegralRangeInvalid() {
    auto presenter = makePresenter();
    auto const range = RangeInLambda(7, 4);

    EXPECT_CALL(m_view, getMonitorIntegralMin()).WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorIntegralMax()).WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorIntegralRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().monitorIntegralRange(), range);
    TS_ASSERT(!presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testSetMonitorBackgroundRange() {
    auto presenter = makePresenter();
    auto const range = RangeInLambda(2.0, 13);

    EXPECT_CALL(m_view, getMonitorBackgroundMin())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorBackgroundMax())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorBackgroundRangeValid()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().monitorBackgroundRange(), range);
    TS_ASSERT(presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testSetMonitorBackgroundRangeInvalid() {
    auto presenter = makePresenter();
    auto const range = RangeInLambda(2.001, 2.000);

    EXPECT_CALL(m_view, getMonitorBackgroundMin())
        .WillOnce(Return(range.min()));
    EXPECT_CALL(m_view, getMonitorBackgroundMax())
        .WillOnce(Return(range.max()));
    EXPECT_CALL(m_view, showMonitorBackgroundRangeInvalid()).Times(1);
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().monitorBackgroundRange(), range);
    TS_ASSERT(!presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testCorrectDetectorsToggled() {
    auto presenter = makePresenter();
    auto const correctDetectors = !presenter.instrument().correctDetectors();

    EXPECT_CALL(m_view, getCorrectDetectors())
        .WillOnce(Return(correctDetectors));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().correctDetectors(),
                     correctDetectors);
    TS_ASSERT(presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testSetDetectorCorrectionType() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, getDetectorCorrectionType())
        .WillOnce(Return("RotateAroundSample"));
    presenter.notifySettingsChanged();

    TS_ASSERT_EQUALS(presenter.instrument().detectorCorrectionType(),
                     DetectorCorrectionType::RotateAroundSample);
    TS_ASSERT(presenter.instrument().isValid());
    TS_ASSERT(verifyAndClear());
  }

  void testAllWidgetsAreEnabledWhenReductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    presenter.onReductionPaused();

    TS_ASSERT(verifyAndClear());
  }

  void testAllWidgetsAreDisabledWhenReductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    presenter.onReductionResumed();

    TS_ASSERT(verifyAndClear());
  }

protected:
  NiceMock<MockInstrumentView> m_view;
};

#endif // MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_
