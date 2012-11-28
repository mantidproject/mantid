#ifndef SLICE_VIEWER_NULLPEAKSPRESENTER_TEST_H_
#define SLICE_VIEWER_NULLPEAKSPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtSliceViewer/NullPeaksPresenter.h"

using namespace MantidQt::SliceViewer;
using namespace Mantid;

class NullPeaksPresenterTest : public CxxTest::TestSuite
{

public:

  void test_construction()
  {
      TS_ASSERT_THROWS_NOTHING(NullPeaksPresenter p());
  }

  void test_is_peaks_presenter()
  {
    NullPeaksPresenter presenter;
    PeaksPresenter& base = presenter; // compile-time test for the is-a relationship
  }

  /* Test individual methods on the interface */

  void test_update_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.update());
  }

  void test_updateWithSlicePoint_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT_THROWS_NOTHING(presenter.updateWithSlicePoint(0));
  }

  void test_changeShownDim_does_nothing()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT(!presenter.changeShownDim());
  }

  void test_isLabelOfFreeAxis_always_returns_false()
  {
    NullPeaksPresenter presenter;
    TS_ASSERT(!presenter.isLabelOfFreeAxis(""));
  }

};

#endif