#ifndef MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H_

#include "MockEventView.h"
#include "../../../ISISReflectometry/GUI/Event/EventPresenter.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::Mock;
using testing::NiceMock;

class EventPresenterTest : public CxxTest::TestSuite {
public:
  static EventPresenterTest *createSuite() { return new EventPresenterTest(); }
  static void destroySuite(EventPresenterTest *suite) { delete suite; }

  EventPresenterTest() : m_view() {}

  EventPresenter makePresenter() { return EventPresenter(&m_view); }

  bool verifyAndClear() {
    return Mock::VerifyAndClearExpectations(&m_view);
  }

  void testNoEventSlicingByDefault() {
    auto presenter = makePresenter();
    TS_ASSERT(isNoSlicing(presenter.slicing()));
    TS_ASSERT(verifyAndClear());
  }

  void testInitializesWithStateFromViewWhenChangingToUniformSlicingByTime() {
    auto presenter = makePresenter();
    auto const secondsPerSlice = 10.0;

    EXPECT_CALL(m_view, uniformSliceLength()).WillOnce(Return(secondsPerSlice));

    presenter.notifySliceTypeChanged(SliceType::Uniform);
    auto const &uniformSlicingByTime =
        boost::get<UniformSlicingByTime>(presenter.slicing());
    TS_ASSERT(uniformSlicingByTime == UniformSlicingByTime(secondsPerSlice));
    TS_ASSERT(verifyAndClear());
  }

  void
  testInitializesWithStateFromViewWhenChangingToUniformSlicingByNumberOfSlices() {
    auto presenter = makePresenter();
    auto const numberOfSlices = 11;

    EXPECT_CALL(m_view, uniformSliceCount()).WillOnce(Return(numberOfSlices));

    presenter.notifySliceTypeChanged(SliceType::UniformEven);
    auto const &uniformSlicingByNumberOfSlices =
        boost::get<UniformSlicingByNumberOfSlices>(presenter.slicing());
    TS_ASSERT(uniformSlicingByNumberOfSlices ==
              UniformSlicingByNumberOfSlices(numberOfSlices));
    TS_ASSERT(verifyAndClear());
  }

  void testInitializesWithStateFromViewWhenChangingToCustomSlicing() {
    auto presenter = makePresenter();
    auto const expectedSliceTimes =
        std::vector<double>({11.0, 12.0, 33.0, 23.2});
    auto const sliceTimeList = std::string("11, 12,33, 23.2");

    EXPECT_CALL(m_view, customBreakpoints()).WillOnce(Return(sliceTimeList));

    presenter.notifySliceTypeChanged(SliceType::Custom);
    auto const &sliceTimes =
        boost::get<CustomSlicingByList>(presenter.slicing());
    TS_ASSERT(sliceTimes == CustomSlicingByList(expectedSliceTimes));
    TS_ASSERT(verifyAndClear());
  }

  void testInitializedWithStateFromViewWhenChangingToSlicingByEventLog() {
    auto presenter = makePresenter();
    auto const logBlockName = std::string("Param");
    auto const expectedSliceValues =
        std::vector<double>({11.0, 0.1, 12.0, 33.0, 23.2});
    auto const sliceValuesList = std::string("11,0.1, 12,33, 23.2");

    EXPECT_CALL(m_view, logBreakpoints()).WillOnce(Return(sliceValuesList));
    EXPECT_CALL(m_view, logBlockName()).WillOnce(Return(logBlockName));

    presenter.notifySliceTypeChanged(SliceType::LogValue);
    auto const &sliceValues =
        boost::get<SlicingByEventLog>(presenter.slicing());
    TS_ASSERT(sliceValues ==
              SlicingByEventLog(expectedSliceValues, logBlockName));
    TS_ASSERT(verifyAndClear());
  }

  void testChangingSliceCountUpdatesModel() {
    auto presenter = makePresenter();
    auto const expectedSliceCount = 10;

    EXPECT_CALL(m_view, uniformSliceCount())
        .WillOnce(Return(0))
        .WillOnce(Return(expectedSliceCount));

    presenter.notifySliceTypeChanged(SliceType::UniformEven);
    presenter.notifyUniformSliceCountChanged(expectedSliceCount);
    auto const &sliceValues =
        boost::get<UniformSlicingByNumberOfSlices>(presenter.slicing());
    TS_ASSERT(sliceValues ==
              UniformSlicingByNumberOfSlices(expectedSliceCount));
    TS_ASSERT(verifyAndClear());
  }

  void testViewUpdatedWhenInvalidSliceValuesEntered() {
    auto presenter = makePresenter();
    auto const invalidCustomBreakpoints = std::string("1,");

    EXPECT_CALL(m_view, customBreakpoints())
        .WillOnce(Return("1"))
        .WillOnce(Return(invalidCustomBreakpoints));

    presenter.notifySliceTypeChanged(SliceType::Custom);

    EXPECT_CALL(m_view, showCustomBreakpointsInvalid());
    presenter.notifyCustomSliceValuesChanged(invalidCustomBreakpoints);
    TS_ASSERT(verifyAndClear());
  }

  void testModelUpdatedWhenInvalidSliceValuesEntered() {
    auto presenter = makePresenter();
    auto const invalidCustomBreakpoints = std::string("1,");

    EXPECT_CALL(m_view, customBreakpoints())
        .WillOnce(Return("1"))
        .WillOnce(Return(invalidCustomBreakpoints));

    presenter.notifySliceTypeChanged(SliceType::Custom);
    presenter.notifyCustomSliceValuesChanged(invalidCustomBreakpoints);
    auto const &slicing = boost::get<InvalidSlicing>(presenter.slicing());
    TS_ASSERT(slicing == InvalidSlicing());
    TS_ASSERT(verifyAndClear());
  }

  void testModelUpdatedWhenInvalidSliceValuesCorrected() {
    auto presenter = makePresenter();
    auto const validCustomBreakpoints = std::string("1");

    EXPECT_CALL(m_view, customBreakpoints())
        .WillOnce(Return("1,"))
        .WillOnce(Return(validCustomBreakpoints));

    presenter.notifySliceTypeChanged(SliceType::Custom);
    presenter.notifyCustomSliceValuesChanged(validCustomBreakpoints);
    TS_ASSERT(!isInvalid(presenter.slicing()));
    TS_ASSERT(verifyAndClear());
  }

  void testViewUpdatedWhenInvalidSliceValuesCorrected() {
    auto presenter = makePresenter();
    auto const validCustomBreakpoints = std::string("1");

    EXPECT_CALL(m_view, customBreakpoints())
        .WillOnce(Return("1,"))
        .WillOnce(Return(validCustomBreakpoints));

    presenter.notifySliceTypeChanged(SliceType::Custom);

    EXPECT_CALL(m_view, showCustomBreakpointsValid());
    presenter.notifyCustomSliceValuesChanged(validCustomBreakpoints);
    TS_ASSERT(verifyAndClear());
  }

private:
  MockEventView m_view;
};
#endif // MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H_
