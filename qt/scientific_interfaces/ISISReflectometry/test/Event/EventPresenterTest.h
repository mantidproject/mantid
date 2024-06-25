// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Event/EventPresenter.h"
#include "../ReflMockObjects.h"
#include "MockEventView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

class EventPresenterTest : public CxxTest::TestSuite {
public:
  static EventPresenterTest *createSuite() { return new EventPresenterTest(); }
  static void destroySuite(EventPresenterTest *suite) { delete suite; }

  EventPresenterTest() : m_view() {}

  void tearDown() override {
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
  }

  void testNoEventSlicingByDefault() {
    auto presenter = makePresenter();
    TS_ASSERT(isNoSlicing(presenter.slicing()));
  }

  void testInitializesWithStateFromViewWhenChangingToUniformSlicingByTime() {
    auto presenter = makePresenter();
    auto const secondsPerSlice = 10.0;
    auto const sliceType = SliceType::Uniform;

    EXPECT_CALL(m_view, uniformSliceLength()).WillOnce(Return(secondsPerSlice));
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    TS_ASSERT_EQUALS(presenter.slicing().which(), 2);
    auto const &uniformSlicingByTime = boost::get<UniformSlicingByTime>(presenter.slicing());
    TS_ASSERT(uniformSlicingByTime == UniformSlicingByTime(secondsPerSlice));
  }

  void testInitializesWithStateFromViewWhenChangingToUniformSlicingByNumberOfSlices() {
    auto presenter = makePresenter();
    auto const numberOfSlices = 11;
    auto const sliceType = SliceType::UniformEven;

    EXPECT_CALL(m_view, uniformSliceCount()).WillOnce(Return(numberOfSlices));
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    TS_ASSERT_EQUALS(presenter.slicing().which(), 3);
    auto const &uniformSlicingByNumberOfSlices = boost::get<UniformSlicingByNumberOfSlices>(presenter.slicing());
    TS_ASSERT(uniformSlicingByNumberOfSlices == UniformSlicingByNumberOfSlices(numberOfSlices));
  }

  void testInitializesWithStateFromViewWhenChangingToCustomSlicing() {
    auto presenter = makePresenter();
    auto const sliceType = SliceType::Custom;
    auto const expectedSliceTimes = std::vector<double>({11.0, 12.0, 33.0, 23.2});
    auto const sliceTimeList = std::string("11, 12,33, 23.2");

    EXPECT_CALL(m_view, customBreakpoints()).WillOnce(Return(sliceTimeList));
    expectChangeSliceType(SliceType::None, sliceType);
    EXPECT_CALL(m_view, showCustomBreakpointsValid());
    presenter.notifySliceTypeChanged(sliceType);

    TS_ASSERT_EQUALS(presenter.slicing().which(), 4);
    auto const &sliceTimes = boost::get<CustomSlicingByList>(presenter.slicing());
    TS_ASSERT(sliceTimes == CustomSlicingByList(expectedSliceTimes));
  }

  void testInitializedWithStateFromViewWhenChangingToSlicingByEventLog() {
    auto presenter = makePresenter();
    auto const sliceType = SliceType::LogValue;
    auto const logBlockName = std::string("Param");
    auto const expectedSliceValues = std::vector<double>({11.0});
    auto const sliceValuesList = std::string("11");

    EXPECT_CALL(m_view, logBreakpoints()).WillOnce(Return(sliceValuesList));
    EXPECT_CALL(m_view, logBlockName()).WillOnce(Return(logBlockName));
    EXPECT_CALL(m_view, showLogBreakpointsValid());
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    TS_ASSERT_EQUALS(presenter.slicing().which(), 5);
    auto const &sliceValues = boost::get<SlicingByEventLog>(presenter.slicing());
    TS_ASSERT(sliceValues == SlicingByEventLog(expectedSliceValues, logBlockName));
  }

  void testChangingSliceCountUpdatesModel() {
    auto presenter = makePresenter();
    auto const sliceType = SliceType::UniformEven;
    auto const expectedSliceCount = 10;

    EXPECT_CALL(m_view, uniformSliceCount()).WillOnce(Return(0)).WillOnce(Return(expectedSliceCount));
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    presenter.notifyUniformSliceCountChanged(expectedSliceCount);
    auto const &sliceValues = boost::get<UniformSlicingByNumberOfSlices>(presenter.slicing());
    TS_ASSERT(sliceValues == UniformSlicingByNumberOfSlices(expectedSliceCount));
  }

  void testViewUpdatedWhenInvalidSliceValuesEntered() {
    auto presenter = makePresenter();
    auto const sliceType = SliceType::Custom;
    auto const invalidCustomBreakpoints = std::string("1,");

    EXPECT_CALL(m_view, customBreakpoints()).WillOnce(Return("1")).WillOnce(Return(invalidCustomBreakpoints));
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    EXPECT_CALL(m_view, showCustomBreakpointsInvalid()).Times(1);
    presenter.notifyCustomSliceValuesChanged(invalidCustomBreakpoints);
  }

  void testModelUpdatedWhenInvalidSliceValuesEntered() {
    auto presenter = makePresenter();
    auto const sliceType = SliceType::Custom;
    auto const invalidCustomBreakpoints = std::string("1,");

    EXPECT_CALL(m_view, customBreakpoints()).WillOnce(Return("1")).WillOnce(Return(invalidCustomBreakpoints));
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    presenter.notifyCustomSliceValuesChanged(invalidCustomBreakpoints);
    auto const &slicing = boost::get<InvalidSlicing>(presenter.slicing());
    TS_ASSERT(slicing == InvalidSlicing());
  }

  void testModelUpdatedWhenInvalidSliceValuesCorrected() {
    auto presenter = makePresenter();
    auto const sliceType = SliceType::Custom;
    auto const validCustomBreakpoints = std::string("1");

    EXPECT_CALL(m_view, customBreakpoints()).WillOnce(Return("1,")).WillOnce(Return(validCustomBreakpoints));
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    presenter.notifyCustomSliceValuesChanged(validCustomBreakpoints);
    TS_ASSERT(!isInvalid(presenter.slicing()));
  }

  void testViewUpdatedWhenInvalidSliceValuesCorrected() {
    auto presenter = makePresenter();
    auto const sliceType = SliceType::Custom;
    auto const validCustomBreakpoints = std::string("1");

    EXPECT_CALL(m_view, customBreakpoints()).WillOnce(Return("1,")).WillOnce(Return(validCustomBreakpoints));
    expectChangeSliceType(SliceType::None, sliceType);
    presenter.notifySliceTypeChanged(sliceType);

    EXPECT_CALL(m_view, showCustomBreakpointsValid());
    presenter.notifyCustomSliceValuesChanged(validCustomBreakpoints);
  }

  void testChangingSliceTypeNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifySliceTypeChanged(SliceType::None);
  }

  void testChangingSliceCountNotifiesMainPresenter() {
    auto presenter = makePresenter();
    presenter.notifySliceTypeChanged(SliceType::UniformEven);
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifyUniformSliceCountChanged(1);
  }

  void testChangingCustomSliceValuesNotifiesMainPresenter() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(AtLeast(1));
    presenter.notifyCustomSliceValuesChanged("");
  }

  void testNoSlicingOccursWhenSliceTypeIsNone() {
    auto presenter = makePresenter();
    presenter.notifySliceTypeChanged(SliceType::None);

    EXPECT_CALL(m_mainPresenter, notifySettingsChanged()).Times(0);
    presenter.notifyUniformSliceCountChanged(2);
    presenter.notifyUniformSecondsChanged(2);
    presenter.notifyCustomSliceValuesChanged("string");
    presenter.notifyLogSliceBreakpointsChanged("");
    presenter.notifyLogBlockNameChanged("");
  }

private:
  NiceMock<MockEventView> m_view;
  NiceMock<MockBatchPresenter> m_mainPresenter;

  EventPresenter makePresenter() {
    auto presenter = EventPresenter(&m_view);
    presenter.acceptMainPresenter(&m_mainPresenter);
    return presenter;
  }

  void verifyAndClear() { TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view)); }

  void expectChangeSliceType(SliceType oldSliceType, SliceType newSliceType) {
    EXPECT_CALL(m_view, disableSliceType(oldSliceType)).Times(1);
    EXPECT_CALL(m_view, enableSliceType(newSliceType)).Times(1);
  }
};