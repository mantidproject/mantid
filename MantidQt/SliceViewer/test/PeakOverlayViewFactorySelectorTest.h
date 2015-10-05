#ifndef SLICE_VIEWER_PEAKOVERLAYVIEWFACTORYSELECTOR_TEST_H_
#define SLICE_VIEWER_PEAKOVERLAYVIEWFACTORYSELECTOR_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MockObjects.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactorySelector.h"

using namespace MantidQt::SliceViewer;
using namespace testing;

class PeakOverlayViewFactorySelectorTest: public CxxTest::TestSuite
{
public:

  void test_construction()
  {
    TS_ASSERT_THROWS_NOTHING(PeakOverlayViewFactorySelector selector);
  }

  void test_registerCandiates()
  {
    PeakOverlayViewFactory_sptr factory1 = boost::make_shared<MockPeakOverlayFactory>();
    PeakOverlayViewFactory_sptr factory2 = boost::make_shared<MockPeakOverlayFactory>();

    PeakOverlayViewFactorySelector selector;
    selector.registerCandidate(factory1);
    selector.registerCandidate(factory2);

    TS_ASSERT_EQUALS(2, selector.countCandidates());
  }

  void test_cannot_double_registerCandidate()
  {
    PeakOverlayViewFactory_sptr candidate = boost::make_shared<MockPeakOverlayFactory>();

    PeakOverlayViewFactorySelector selector;
    selector.registerCandidate(candidate);
    selector.registerCandidate(candidate);

    TS_ASSERT_EQUALS(1, selector.countCandidates());
  }

  void test_makeSelection_throws_if_nothing_registered()
  {
    PeakOverlayViewFactorySelector selector;

    TS_ASSERT_EQUALS(0, selector.countCandidates());
    TS_ASSERT_THROWS(selector.makeSelection(), std::logic_error&);
  }

  void test_makeSelection_throws_if_there_are_no_good_options()
  {
    MockPeakOverlayFactory*  pCandidate = new MockPeakOverlayFactory;
    EXPECT_CALL(*pCandidate, FOM()).Times(1).WillOnce(Return(0)); // A zero return means that this factory is not suitable.
    PeakOverlayViewFactory_sptr candidate(pCandidate);

    PeakOverlayViewFactorySelector selector;
    selector.registerCandidate(candidate);
    TS_ASSERT_EQUALS(1, selector.countCandidates());
    TS_ASSERT_THROWS(selector.makeSelection(), std::logic_error&);
  }

  void test_makeSelection()
  {
    MockPeakOverlayFactory* pCandidate1 = new MockPeakOverlayFactory;
    EXPECT_CALL(*pCandidate1, FOM()).Times(1).WillOnce(Return(1)); // Returns a FOM of 1
    PeakOverlayViewFactory_sptr candidate1(pCandidate1);

    MockPeakOverlayFactory* pCandidate2 = new MockPeakOverlayFactory;
    EXPECT_CALL(*pCandidate2, FOM()).Times(1).WillOnce(Return(2)); // Returns a FOM of 2, so should win.
    PeakOverlayViewFactory_sptr candidate2(pCandidate2);

    PeakOverlayViewFactorySelector selector;
    selector.registerCandidate(candidate1);
    selector.registerCandidate(candidate2);

    TS_ASSERT_EQUALS(2, selector.countCandidates());
    TS_ASSERT( candidate2 == selector.makeSelection());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pCandidate1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pCandidate2));
  }

};

#endif
