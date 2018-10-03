// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SLICE_VIEWER_NULLPEAKSPRESENTER_TEST_H_
#define SLICE_VIEWER_NULLPEAKSPRESENTER_TEST_H_

#include "MantidQtWidgets/SliceViewer/NullPeaksPresenter.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::SliceViewer;

class NullPeaksPresenterTest : public CxxTest::TestSuite {

public:
  void test_construction() { TS_ASSERT_THROWS_NOTHING(NullPeaksPresenter p); }

  void test_is_peaks_presenter() {
    NullPeaksPresenter presenter;
    PeaksPresenter &base =
        presenter; // compile-time test for the is-a relationship
    UNUSED_ARG(base);
  }

  /* Test individual methods on the interface */

  void test_update_does_nothing() {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.update());
  }

  void test_updateWithSlicePoint_does_nothing() {
    NullPeaksPresenter presenter;
    PeakBoundingBox region;
    TS_ASSERT_THROWS_NOTHING(presenter.updateWithSlicePoint(region));
  }

  void test_changeShownDim_does_nothing() {
    NullPeaksPresenter presenter;
    TS_ASSERT(!presenter.changeShownDim(0, 1));
  }

  void test_isLabelOfFreeAxis_always_returns_false() {
    NullPeaksPresenter presenter;
    TS_ASSERT(!presenter.isLabelOfFreeAxis(""));
  }

  void test_presentedWorkspaces_is_empty() {
    NullPeaksPresenter presenter;
    SetPeaksWorkspaces workspaces = presenter.presentedWorkspaces();
    TS_ASSERT_EQUALS(0, workspaces.size());
  }

  void test_setForegroundColour_does_nothing() {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setForegroundColor(PeakViewColor()));
  }

  void test_setBackgroundColour_does_nothing() {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setBackgroundColor(PeakViewColor()));
  }

  void test_setShown_does_nothing() {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setShown(true));
    TS_ASSERT_THROWS_NOTHING(presenter.setShown(false));
  }

  /// return a box collapsed to a point at 0, 0.
  void test_getBoundingBox_returns_point() {
    NullPeaksPresenter presenter;
    PeakBoundingBox result = presenter.getBoundingBox(0);

    TS_ASSERT_EQUALS(0, result.left());
    TS_ASSERT_EQUALS(0, result.right());
    TS_ASSERT_EQUALS(0, result.top());
    TS_ASSERT_EQUALS(0, result.bottom());
  }

  void test_setPeakSizeOnProjection() {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setPeakSizeOnProjection(1));
  }

  void test_setPeakSizeIntoProjection() {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.setPeakSizeIntoProjection(1));
  }

  void test_getPeakSizeOnProjection() {
    NullPeaksPresenter presenter;
    TS_ASSERT_EQUALS(0, presenter.getPeakSizeOnProjection());
  }

  void test_getPeakSizeIntoProjection() {
    NullPeaksPresenter presenter;
    TS_ASSERT_EQUALS(0, presenter.getPeakSizeIntoProjection());
  }

  void test_contentsDifferent() {
    NullPeaksPresenter presenter;
    MockPeaksPresenter other;
    TS_ASSERT(presenter.contentsDifferent(&other));
  }

  void test_peakEditMode() {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.peakEditMode(AddPeaks););
  }

  void test_deletePeaksIn() {
    NullPeaksPresenter presenter;
    PeakBoundingBox fake;
    TS_ASSERT_THROWS_NOTHING(presenter.deletePeaksIn(fake));
  }
};

#endif
