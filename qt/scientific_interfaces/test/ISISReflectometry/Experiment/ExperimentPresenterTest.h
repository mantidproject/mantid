#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Experiment/ExperimentPresenter.h"
#include "MockExperimentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class ExperimentPresenterTest : public CxxTest::TestSuite {
  using OptionsRow = std::array<std::string, 8>;
  using OptionsTable = std::vector<OptionsRow>;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentPresenterTest *createSuite() {
    return new ExperimentPresenterTest();
  }
  static void destroySuite(ExperimentPresenterTest *suite) { delete suite; }

  ExperimentPresenterTest() : m_view() {}

  void testAllWidgetsAreEnabledWhenReductionPaused() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, enableAll()).Times(1);
    presenter.onReductionPaused();

    verifyAndClear();
  }

  void testAllWidgetsAreDisabledWhenReductionResumed() {
    auto presenter = makePresenter();

    EXPECT_CALL(m_view, disableAll()).Times(1);
    presenter.onReductionResumed();

    verifyAndClear();
  }

  void testModelUpdatedWhenSummationTypeChanged() {
    auto presenter = makePresenter();

    expectViewReturnsDefaultValuesForSumInQ();
    presenter.notifySummationTypeChanged();

    TS_ASSERT_EQUALS(presenter.experiment().summationType(),
                     SummationType::SumInQ);
    verifyAndClear();
  }

  void testReductionTypeDisabledWhenChangeToSumInLambda() {
    auto presenter = makePresenter();

    expectViewReturnsDefaultValues();
    EXPECT_CALL(m_view, disableReductionType()).Times(1);
    presenter.notifySummationTypeChanged();

    verifyAndClear();
  }

  void testReductionTypeEnbledWhenChangeToSumInQ() {
    auto presenter = makePresenter();

    expectViewReturnsDefaultValuesForSumInQ();
    EXPECT_CALL(m_view, enableReductionType()).Times(1);
    presenter.notifySummationTypeChanged();

    verifyAndClear();
  }

  void testNewPerAngleDefaultsRequested() {
    auto presenter = makePresenter();

    // row should be added to view
    EXPECT_CALL(m_view, addPerThetaDefaultsRow());
    // new value should be requested from view to update model
    expectViewReturnsDefaultValues();
    EXPECT_CALL(m_view, getPerAngleOptions()).Times(1);
    presenter.notifyNewPerAngleDefaultsRequested();

    verifyAndClear();
  }

  void testRemovePerAngleDefaultsRequested() {
    auto presenter = makePresenter();

    int const indexToRemove = 0;
    // row should be removed from view
    EXPECT_CALL(m_view, removePerThetaDefaultsRow(indexToRemove)).Times(1);
    // new value should be requested from view to update model
    expectViewReturnsDefaultValues();
    EXPECT_CALL(m_view, getPerAngleOptions()).Times(1);
    presenter.notifyRemovePerAngleDefaultsRequested(indexToRemove);

    verifyAndClear();
  }

  void testChangingPerAngleDefaultsUpdatesModel() {
    auto presenter = makePresenter();

    auto const row = 1;
    auto const column = 2;
    OptionsTable const optionsTable = {optionsWithAngleAndOneTrans(),
                                       optionsWithAngleAndTwoTrans()};
    expectViewReturnsDefaultValues();
    EXPECT_CALL(m_view, getPerAngleOptions()).WillOnce(Return(optionsTable));
    presenter.notifyPerAngleDefaultsChanged(row, column);

    // Check the model contains the per-theta defaults returned by the view
    auto const perThetaDefaults = presenter.experiment().perThetaDefaults();
    TS_ASSERT_EQUALS(perThetaDefaults.size(), 2);
    TS_ASSERT_EQUALS(perThetaDefaults[0], defaultsWithAngleAndOneTrans());
    TS_ASSERT_EQUALS(perThetaDefaults[1], defaultsWithAngleAndTwoTrans());
    verifyAndClear();
  }

private:
  NiceMock<MockExperimentView> m_view;

  ExperimentPresenter makePresenter() {
    // The presenter gets values from the view on construction so the view must
    // return something sensible
    expectViewReturnsDefaultValues();
    auto presenter = ExperimentPresenter(&m_view, /*thetaTolerance=*/0.01);
    verifyAndClear();
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }

  void expectViewReturnsDefaultValues() {
    EXPECT_CALL(m_view, getAnalysisMode())
        .WillOnce(Return(std::string("PointDetectorAnalysis")));
    EXPECT_CALL(m_view, getSummationType())
        .WillOnce(Return(std::string("SumInLambda")));
    EXPECT_CALL(m_view, getReductionType())
        .WillOnce(Return(std::string("Normal")));
  }

  void expectViewReturnsDefaultValuesForSumInQ() {
    EXPECT_CALL(m_view, getAnalysisMode())
        .WillOnce(Return(std::string("PointDetectorAnalysis")));
    EXPECT_CALL(m_view, getSummationType())
        .WillOnce(Return(std::string("SumInQ")));
    EXPECT_CALL(m_view, getReductionType())
        .WillOnce(Return(std::string("DivergentBeam")));
  }

  // These functions create various rows in the per-theta defaults tables,
  // either as an input array of strings or an output model
  OptionsRow optionsWithAngleAndOneTrans() { return {"0.5", "13463"}; }
  PerThetaDefaults defaultsWithAngleAndOneTrans() {
    return PerThetaDefaults(0.5, std::make_pair("13463", ""),
                            boost::optional<RangeInQ>(),
                            boost::optional<double>(), ReductionOptionsMap());
  }

  OptionsRow optionsWithAngleAndTwoTrans() { return {"2.3", "13463", "13464"}; }
  PerThetaDefaults defaultsWithAngleAndTwoTrans() {
    return PerThetaDefaults(2.3, std::make_pair("13463", "13464"),
                            boost::optional<RangeInQ>(),
                            boost::optional<double>(), ReductionOptionsMap());
  }
};

#endif // MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_
