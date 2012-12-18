#ifndef SLICE_VIEWER_COMPOSITEPEAKSPRESENTER_TEST_H_
#define SLICE_VIEWER_COMPOSITEPEAKSPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>
#include "MantidQtSliceViewer/CompositePeaksPresenter.h"
#include "MantidQtSliceViewer/NullPeaksPresenter.h"
#include "MockObjects.h"

using namespace MantidQt::SliceViewer;
using namespace Mantid;
using namespace testing;

class CompositePeaksPresenterTest : public CxxTest::TestSuite
{

public:

  void test_construction()
  {
     CompositePeaksPresenter composite;
     TSM_ASSERT_EQUALS("Should default construct with a NullPeaksPresenter", 0, composite.size());

     /*After default construction, the composite presenter should behave identically to a NullPeaks presenter.*/
     NullPeaksPresenter expected;
     TS_ASSERT_THROWS_NOTHING(expected.update());
     TS_ASSERT_THROWS_NOTHING(composite.update());
     TS_ASSERT_THROWS_NOTHING(expected.updateWithSlicePoint(0));
     TS_ASSERT_THROWS_NOTHING(composite.updateWithSlicePoint(0));
     TS_ASSERT_EQUALS(expected.changeShownDim(), composite.changeShownDim());
     TS_ASSERT_EQUALS(expected.isLabelOfFreeAxis("") , composite.isLabelOfFreeAxis(""));
  }

  void test_add_peaks_presenter()
  {
    CompositePeaksPresenter presenter;
    const size_t initialSize = presenter.size();
    presenter.addPeaksPresenter( boost::make_shared<MockPeaksPresenter>() );
    TSM_ASSERT_EQUALS("Expected one item to be added.", initialSize + 1, presenter.size());
  }

  void test_keep_presenters_unique()
  {
    CompositePeaksPresenter presenter;
    const size_t initialSize = presenter.size();
    auto presenterToAdd = boost::make_shared<MockPeaksPresenter>();
    presenter.addPeaksPresenter( presenterToAdd );
    presenter.addPeaksPresenter( presenterToAdd ); // Try to add it again.
    TSM_ASSERT_EQUALS("Should not be able to add the same item more than once.", initialSize + 1, presenter.size());
  }

  void test_clear()
  {
    CompositePeaksPresenter composite;
    const size_t initialSize = composite.size();
    composite.addPeaksPresenter( boost::make_shared<MockPeaksPresenter>() ); // Add one subject
    composite.addPeaksPresenter( boost::make_shared<MockPeaksPresenter>() ); // Add another subject

    composite.clear();

    TSM_ASSERT_EQUALS("Should be back to initial size after clearing.", initialSize, composite.size());

    /*After clearing, the composite presenter should behave identically to a NullPeaks presenter.*/
    NullPeaksPresenter expected;
    TS_ASSERT_THROWS_NOTHING(expected.update());
    TS_ASSERT_THROWS_NOTHING(composite.update());
    TS_ASSERT_THROWS_NOTHING(expected.updateWithSlicePoint(0));
    TS_ASSERT_THROWS_NOTHING(composite.updateWithSlicePoint(0));
    TS_ASSERT_EQUALS(expected.changeShownDim(), composite.changeShownDim());
    TS_ASSERT_EQUALS(expected.isLabelOfFreeAxis("") , composite.isLabelOfFreeAxis(""));
  }

  /**
  Check that when no subject presenters have been added, that the composite uses the 'default' for the updateWithSlicePoint method.
  */
  void test_updateWithSlicePoint_default()
  {
    // Create a default.
    MockPeaksPresenter* mockDefault = new MockPeaksPresenter;
    PeaksPresenter_sptr defaultPresenter(mockDefault);
    EXPECT_CALL(*mockDefault, updateWithSlicePoint(_)).Times(1); // Expect the method on the default to be called.

    // Create the composite.
    CompositePeaksPresenter composite(defaultPresenter);
    // Call the method on the composite.
    composite.updateWithSlicePoint(0);

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockDefault));
  }

  void test_updateWithSlicePoint()
  {
    MockPeaksPresenter* mockPresenter = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter(mockPresenter);
    EXPECT_CALL(*mockPresenter, updateWithSlicePoint(_)).Times(1); // Expect the method on the default to be called.

    // Create the composite.
    CompositePeaksPresenter composite;
    // add the subject presenter.
    composite.addPeaksPresenter(presenter);
    // Call the method on the composite.
    composite.updateWithSlicePoint(0);

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockPresenter));
  }

  /**
  Check that when no subject presenters have been added, that the composite uses the 'default' for the update method.
  */
  void test_update_default()
  {
    // Create a default.
    MockPeaksPresenter* mockDefault = new MockPeaksPresenter;
    PeaksPresenter_sptr defaultPresenter(mockDefault);
    EXPECT_CALL(*mockDefault, update()).Times(1); // Expect the method on the default to be called.

    // Create the composite.
    CompositePeaksPresenter composite(defaultPresenter);
    // Call the method on the composite.
    composite.update();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockDefault));
  }

  void test_update()
  {
    MockPeaksPresenter* mockPresenter = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter(mockPresenter);
    EXPECT_CALL(*mockPresenter, update()).Times(1); // Expect the method on the default to be called.

    // Create the composite.
    CompositePeaksPresenter composite;
    // add the subject presenter.
    composite.addPeaksPresenter(presenter);
    // Call the method on the composite.
    composite.update();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockPresenter));
  }

  void test_presentedWorkspaces()
  {
    //One nested presenter
    SetPeaksWorkspaces setA;
    MockPeaksPresenter* pA = new MockPeaksPresenter;
    PeaksPresenter_sptr A(pA);
    EXPECT_CALL(*pA, presentedWorkspaces()).WillOnce(Return(setA)); 
    
    //Another nested presenter
    SetPeaksWorkspaces setB;
    MockPeaksPresenter* pB = new MockPeaksPresenter;
    PeaksPresenter_sptr B(pB);
    EXPECT_CALL(*pB, presentedWorkspaces()).WillOnce(Return(setB)); 

    // Create the composite.
    CompositePeaksPresenter composite;
    // add the subject presenter.
    composite.addPeaksPresenter(A);
    composite.addPeaksPresenter(B);
    // Call the method on the composite.
    composite.presentedWorkspaces();

    TS_ASSERT(Mock::VerifyAndClearExpectations(pA));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pB));
  }

  void test_changeShownDimension()
  {
    const bool PASS = true;
    const bool FAIL = false;
    
    CompositePeaksPresenter composite;
    
    MockPeaksPresenter* A = new MockPeaksPresenter;
    MockPeaksPresenter* B = new MockPeaksPresenter;
    PeaksPresenter_sptr subjectA(A);
    PeaksPresenter_sptr subjectB(B);

    // if both subjects FAIL, composite should report FAIL
    EXPECT_CALL(*A, changeShownDim()).WillOnce(Return(FAIL));
    EXPECT_CALL(*B, changeShownDim()).WillOnce(Return(FAIL));

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT_EQUALS("Should return FAIL, because both of the subjects FAIL", FAIL, composite.changeShownDim()); 
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
    composite.clear();

    // if one subject FAIL, composite should FAIL.
    EXPECT_CALL(*A, changeShownDim()).WillOnce(Return(PASS));
    EXPECT_CALL(*B, changeShownDim()).WillOnce(Return(FAIL));

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT_EQUALS("Should return FAIL, because at least one of the subjects return FAIL", FAIL, composite.changeShownDim());
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
    composite.clear();

    // if subjects both PASS, composite should PASS.
    EXPECT_CALL(*A, changeShownDim()).WillOnce(Return(PASS));
    EXPECT_CALL(*B, changeShownDim()).WillOnce(Return(PASS));

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT("Should return PASS, because both of the subject PASS", composite.changeShownDim());
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
  }

  void test_isLabelOfFreeAxis()
  {
    const bool PASS = true;
    const bool FAIL = false;
    
    CompositePeaksPresenter composite;
    
    MockPeaksPresenter* A = new MockPeaksPresenter;
    MockPeaksPresenter* B = new MockPeaksPresenter;
    PeaksPresenter_sptr subjectA(A);
    PeaksPresenter_sptr subjectB(B);

    // if both subjects FAIL, composite should report FAIL
    EXPECT_CALL(*A, isLabelOfFreeAxis(_)).WillOnce(Return(FAIL));
    EXPECT_CALL(*B, isLabelOfFreeAxis(_)).WillOnce(Return(FAIL));

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT("Should return FAIL, because both of the subjects FAIL", FAIL == composite.isLabelOfFreeAxis("")); 
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
    composite.clear();

    // if one subject FAIL, composite should FAIL.
    EXPECT_CALL(*A, isLabelOfFreeAxis(_)).WillOnce(Return(PASS));
    EXPECT_CALL(*B, isLabelOfFreeAxis(_)).WillOnce(Return(FAIL));

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT("Should return FAIL, because at least one of the subjects return FAIL", FAIL == composite.isLabelOfFreeAxis(""));
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
    composite.clear();

    // if subjects both PASS, composite should PASS.
    EXPECT_CALL(*A, isLabelOfFreeAxis(_)).WillOnce(Return(PASS));
    EXPECT_CALL(*B, isLabelOfFreeAxis(_)).WillOnce(Return(PASS));

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT("Should return PASS, because both of the subject PASS", PASS == composite.isLabelOfFreeAxis(""));
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
  }

  void test_maximum_allowed_peaks()
  {
    CompositePeaksPresenter presenter;
    // Add peaksWS
    const int limit = 10;
    for(int i = 0; i < limit; ++i)
    {
     TS_ASSERT_THROWS_NOTHING(presenter.addPeaksPresenter( boost::make_shared<MockPeaksPresenter>()));
    }
    // Add a peaksWS beyond the limit of allowed number of peaksWS.
    TS_ASSERT_THROWS(presenter.addPeaksPresenter( boost::make_shared<MockPeaksPresenter>()), std::invalid_argument);
  }

};

#endif
