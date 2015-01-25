#ifndef SLICE_VIEWER_COMPOSITEPEAKSPRESENTER_TEST_H_
#define SLICE_VIEWER_COMPOSITEPEAKSPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>
#include "MantidQtSliceViewer/CompositePeaksPresenter.h"
#include "MantidQtSliceViewer/NullPeaksPresenter.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MockObjects.h"

using namespace MantidQt::SliceViewer;
using namespace Mantid;
using namespace testing;
using Mantid::API::IPeaksWorkspace_sptr;

class CompositePeaksPresenterTest : public CxxTest::TestSuite
{

private:

  /// Fake class to make objects of type
  class FakeZoomablePeaksView : public ZoomablePeaksView
  {
  public:
    void zoomToRectangle(const PeakBoundingBox&){}
    void resetView(){}
    void detach(){}
    virtual ~FakeZoomablePeaksView(){}
  };

  /// Fake zoomable peaks view instance to use as a dummy parameter.
  FakeZoomablePeaksView _fakeZoomableView;

public:

  void test_construction_throws_if_zoomablePeakView__NULL()
  {
    TS_ASSERT_THROWS(CompositePeaksPresenter composite(NULL), std::runtime_error&);
  }


  void test_construction()
  {
     CompositePeaksPresenter composite(&_fakeZoomableView);
     TSM_ASSERT_EQUALS("Should default construct with a &_fakeZoomableViewPeaksPresenter", 0, composite.size());

     /*After default construction, the composite presenter should behave identically to a NULL peaks presenter.*/
     NullPeaksPresenter expected;
     TS_ASSERT_THROWS_NOTHING(expected.update());
     TS_ASSERT_THROWS_NOTHING(composite.update());
     PeakBoundingBox region;
     TS_ASSERT_THROWS_NOTHING(expected.updateWithSlicePoint(region));
     TS_ASSERT_THROWS_NOTHING(composite.updateWithSlicePoint(region));
     TS_ASSERT_EQUALS(expected.changeShownDim(), composite.changeShownDim());
     TS_ASSERT_EQUALS(expected.isLabelOfFreeAxis("") , composite.isLabelOfFreeAxis(""));
  }

  void test_add_peaks_presenter()
  {
    CompositePeaksPresenter presenter(&_fakeZoomableView);
    const size_t initialSize = presenter.size();

    auto candidate = boost::make_shared<NiceMock<MockPeaksPresenter> >();
    EXPECT_CALL(*candidate, contentsDifferent(_)).WillOnce(Return(true));

    presenter.addPeaksPresenter( candidate );
    TSM_ASSERT_EQUALS("Expected one item to be added.", initialSize + 1, presenter.size());
  }

  void test_keep_presenters_unique()
  {
    CompositePeaksPresenter presenter(&_fakeZoomableView);
    const size_t initialSize = presenter.size();
    auto presenterToAdd = boost::make_shared<NiceMock<MockPeaksPresenter> >();
    EXPECT_CALL(*presenterToAdd, contentsDifferent(_)).WillRepeatedly(Return(true));
    presenter.addPeaksPresenter( presenterToAdd );
    presenter.addPeaksPresenter( presenterToAdd ); // Try to add it again.
    TSM_ASSERT_EQUALS("Should not be able to add the same item more than once.", initialSize + 1, presenter.size());
  }

  void test_clear()
  {
    NiceMock<MockZoomablePeaksView> mockZoomableView;
    EXPECT_CALL(mockZoomableView, detach()).Times(1); // Should detach itself when no nested presenters are present.

    CompositePeaksPresenter composite(&mockZoomableView);
    const size_t initialSize = composite.size();
    auto a = boost::make_shared<NiceMock<MockPeaksPresenter> >();
    EXPECT_CALL(*a, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allow us to add the subject
    auto b = boost::make_shared<NiceMock<MockPeaksPresenter> >();
    EXPECT_CALL(*b, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add the subject

    composite.addPeaksPresenter(a); // Add one subject
    composite.addPeaksPresenter(b); // Add another subject

    composite.clear();

    TSM_ASSERT_EQUALS("Should be back to initial size after clearing.", initialSize, composite.size());

    /*After clearing, the composite presenter should behave identically to a &_fakeZoomableViewPeaks presenter.*/
    NullPeaksPresenter expected;
    TS_ASSERT_THROWS_NOTHING(expected.update());
    TS_ASSERT_THROWS_NOTHING(composite.update());
    PeakBoundingBox region;
    TS_ASSERT_THROWS_NOTHING(expected.updateWithSlicePoint(region));
    TS_ASSERT_THROWS_NOTHING(composite.updateWithSlicePoint(region));
    TS_ASSERT_EQUALS(expected.changeShownDim(), composite.changeShownDim());
    TS_ASSERT_EQUALS(expected.isLabelOfFreeAxis("") , composite.isLabelOfFreeAxis(""));
    TSM_ASSERT("Should have detached upon clear", Mock::VerifyAndClearExpectations(&mockZoomableView));
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
    CompositePeaksPresenter composite(&_fakeZoomableView, defaultPresenter);
    // Call the method on the composite.
    PeakBoundingBox region;
    composite.updateWithSlicePoint(region);

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockDefault));
  }

  void test_updateWithSlicePoint()
  {
    MockPeaksPresenter* mockPresenter = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter(mockPresenter);
    EXPECT_CALL(*mockPresenter, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite

    EXPECT_CALL(*mockPresenter, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*mockPresenter, updateWithSlicePoint(_)).Times(1); // Expect the method on the default to be called.

    // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
    // add the subject presenter.

    composite.addPeaksPresenter(presenter);
    // Call the method on the composite.
    PeakBoundingBox region;
    composite.updateWithSlicePoint(region);

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockPresenter));
  }

  void test_getTransformName_default()
  {
    MockPeaksPresenter* mockDefault = new MockPeaksPresenter;
    PeaksPresenter_sptr defaultPresenter(mockDefault);
    EXPECT_CALL(*mockDefault, getTransformName()).Times(1).WillOnce(Return(""));// Expect the method on the default to be called.

    defaultPresenter->getTransformName();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockDefault));
  }

  void test_getTransformName()
  {
    MockPeaksPresenter* mockPresenter = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter(mockPresenter);
    EXPECT_CALL(*mockPresenter, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*mockPresenter, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*mockPresenter, getTransformName()).Times(1).WillOnce(Return("")); 

    // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
    // add the subject presenter.
    composite.addPeaksPresenter(presenter);
    // Call the method on the composite.
    composite.getTransformName();

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
    CompositePeaksPresenter composite(&_fakeZoomableView, defaultPresenter);
    // Call the method on the composite.
    composite.update();

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockDefault));
  }

  void test_update()
  {
    MockPeaksPresenter* mockPresenter = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter(mockPresenter);
    EXPECT_CALL(*mockPresenter, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite

    EXPECT_CALL(*mockPresenter, update()).Times(1); // Expect the method on the default to be called.
    EXPECT_CALL(*mockPresenter, registerOwningPresenter(_)).Times(AtLeast(1));

    // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
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
    EXPECT_CALL(*pA, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    PeaksPresenter_sptr A(pA);
    EXPECT_CALL(*pA, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pA, presentedWorkspaces()).WillOnce(Return(setA)); 
    
    //Another nested presenter
    SetPeaksWorkspaces setB;
    MockPeaksPresenter* pB = new MockPeaksPresenter;
    EXPECT_CALL(*pB, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    PeaksPresenter_sptr B(pB);
    EXPECT_CALL(*pB, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pB, presentedWorkspaces()).WillOnce(Return(setB)); 


    // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
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
    
    CompositePeaksPresenter composite(&_fakeZoomableView);
    
    MockPeaksPresenter* A = new NiceMock<MockPeaksPresenter>();
    MockPeaksPresenter* B = new NiceMock<MockPeaksPresenter>();
    EXPECT_CALL(*A, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*B, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
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
    EXPECT_CALL(*A, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*B, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT_EQUALS("Should return FAIL, because at least one of the subjects return FAIL", FAIL, composite.changeShownDim());
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
    composite.clear();

    // if subjects both PASS, composite should PASS.
    EXPECT_CALL(*A, changeShownDim()).WillOnce(Return(PASS));
    EXPECT_CALL(*B, changeShownDim()).WillOnce(Return(PASS));
    EXPECT_CALL(*A, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*B, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite

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
    
    CompositePeaksPresenter composite(&_fakeZoomableView);
    
    MockPeaksPresenter* A = new NiceMock<MockPeaksPresenter>();
    MockPeaksPresenter* B = new NiceMock<MockPeaksPresenter>();
    EXPECT_CALL(*A, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*B, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
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
    EXPECT_CALL(*A, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*B, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT("Should return FAIL, because at least one of the subjects return FAIL", FAIL == composite.isLabelOfFreeAxis(""));
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
    composite.clear();

    // if subjects both PASS, composite should PASS.
    EXPECT_CALL(*A, isLabelOfFreeAxis(_)).WillOnce(Return(PASS));
    EXPECT_CALL(*B, isLabelOfFreeAxis(_)).WillOnce(Return(PASS));
    EXPECT_CALL(*A, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*B, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite

    composite.addPeaksPresenter(subjectA);
    composite.addPeaksPresenter(subjectB);

    TSM_ASSERT("Should return PASS, because both of the subject PASS", PASS == composite.isLabelOfFreeAxis(""));
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
  }

  void test_maximum_allowed_peaks()
  {
    CompositePeaksPresenter presenter(&_fakeZoomableView);
    // Add peaksWS
    const int limit = 10;
    for(int i = 0; i < limit; ++i)
    {
     auto subject = boost::make_shared<NiceMock<MockPeaksPresenter> >();
     EXPECT_CALL(*subject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
     TS_ASSERT_THROWS_NOTHING(presenter.addPeaksPresenter(subject));
    }


    // Add a peaksWS beyond the limit of allowed number of peaksWS.

    TS_ASSERT_THROWS(presenter.addPeaksPresenter(boost::make_shared<NiceMock<MockPeaksPresenter> >()), std::invalid_argument&);
  }

  void test_default_palette()
  {
    PeakPalette actualDefaultPalette;

    CompositePeaksPresenter presenter(&_fakeZoomableView);
    PeakPalette presenterDefaultPalette = presenter.getPalette();

    TSM_ASSERT_EQUALS("CompositePeaksPresenter should be using a default palette until changed.", actualDefaultPalette, presenterDefaultPalette);
  }

  void test_set_background_colour()
  {
    const QColor newColour = Qt::red;

    // Prepare subject objects.
    Mantid::API::IPeaksWorkspace_sptr peaksWS = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(); 
    SetPeaksWorkspaces set;
    set.insert(peaksWS);
    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pSubject, setBackgroundColor(newColour)).Times(1);
    EXPECT_CALL(*pSubject, presentedWorkspaces()).WillOnce(Return(set));

    // Set a background colour on the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);
    composite.setBackgroundColour(peaksWS, newColour);

    // Check that the internal palette has been correctly updated.
    PeakPalette updatedPalette = composite.getPalette();
    TS_ASSERT_EQUALS(newColour, updatedPalette.backgroundIndexToColour(0));

    // Check that the colour was correctly set on the subject presenter.
    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
  }

  void test_set_foreground_colour()
  {
    const QColor newColour = Qt::red;

    // Prepare subject objects.
    Mantid::API::IPeaksWorkspace_sptr peaksWS = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(); 
    SetPeaksWorkspaces set;
    set.insert(peaksWS);
    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, setForegroundColor(newColour)).Times(1);
    EXPECT_CALL(*pSubject, presentedWorkspaces()).WillOnce(Return(set));

    // Set a background colour on the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);
    composite.setForegroundColour(peaksWS, newColour);

    // Check that the internal palette has been correctly updated.
    PeakPalette updatedPalette = composite.getPalette();
    TS_ASSERT_EQUALS(newColour, updatedPalette.foregroundIndexToColour(0));

    // Check that the colour was correctly set on the subject presenter.
    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
  }

  void test_remove()
  {
    // Create a subject presenter that will be deleted.
    auto A = new NiceMock<DyingMockPeaksPresenter>();
    // Create a subject presenter that won't be deleted.
    auto B = new NiceMock<DyingMockPeaksPresenter>();

    {
      // Create some input peaks workspaces.
      IPeaksWorkspace_sptr peaksWS_A = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(); 
      SetPeaksWorkspaces setA;
      setA.insert(peaksWS_A);

      IPeaksWorkspace_sptr peaksWS_B = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(); 
      SetPeaksWorkspaces setB;
      setB.insert(peaksWS_B);

      PeaksPresenter_sptr subjectA(A);
      EXPECT_CALL(*A, presentedWorkspaces()).WillRepeatedly(Return(setA));
      EXPECT_CALL(*A, die()).Times(1); // This will be called on destruction, because we will foreably remove this presenter!

      PeaksPresenter_sptr subjectB(B);
      EXPECT_CALL(*B, presentedWorkspaces()).WillRepeatedly(Return(setB));
      EXPECT_CALL(*B, die()).Times(1); // This will be called on destruction, because we will foreably remove this presenter!

      MockZoomablePeaksView mockZoomablePeaksView;

      // Create the composite
      CompositePeaksPresenter composite(&mockZoomablePeaksView);
      EXPECT_CALL(*A, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
      EXPECT_CALL(*B, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
      composite.addPeaksPresenter(subjectA);
      composite.addPeaksPresenter(subjectB);

      const size_t preRemovalSize = composite.size(); // benchmark the current size.

      // Remove one of the presenters via its workspace.
      composite.remove(peaksWS_A);

      TSM_ASSERT_EQUALS("A presenter should have been removed.",preRemovalSize, composite.size() + 1);

      // Expect the composite to detach itself when everything is removed.
      EXPECT_CALL(mockZoomablePeaksView, detach()).Times(1);

      // Remove the other presenter via its workspace.
      composite.remove(peaksWS_B);

      TSM_ASSERT_EQUALS("A presenter should have been removed.",preRemovalSize, composite.size() + 2);

      TSM_ASSERT("Composite should have detached itself", Mock::VerifyAndClearExpectations(&mockZoomablePeaksView));
    }
    // Check that the correct presenter has been removed.
    TS_ASSERT(Mock::VerifyAndClearExpectations(A));
    TS_ASSERT(Mock::VerifyAndClearExpectations(B));
  }

  void test_remove_default()
  {
    CompositePeaksPresenter composite(&_fakeZoomableView);
    auto peaksWorkspace = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();

    //Try to remove a peaks workspace & associated presenter that doesn't exist from a default constructed composite.
    TS_ASSERT_THROWS_NOTHING(composite.remove(peaksWorkspace));
  }


  void do_test_setShown(bool expectedToShow)
  {
    // Prepare subject objects.
    Mantid::API::IPeaksWorkspace_sptr peaksWS = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(); 
    SetPeaksWorkspaces set;
    set.insert(peaksWS);
    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, setShown(expectedToShow)).Times(1);
    EXPECT_CALL(*pSubject, presentedWorkspaces()).WillOnce(Return(set));

    // Create the composite and add the test presenter.
    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);

    // execute setshown(...)
    composite.setShown(peaksWS, expectedToShow);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
  }

  void test_setShown()
  {
    const bool SHOW = true;
    const bool HIDE = false;
    // Test that calling method on composite causes subject presenters to show.
    do_test_setShown(SHOW); 
    // Test that calling method on composite causes subject presenters to hide.
    do_test_setShown(HIDE); 
  }

  void test_setShown_default()
  {
    const bool expectedFlag = true;

    // Create a default.
    MockPeaksPresenter* mockDefault = new MockPeaksPresenter;
    PeaksPresenter_sptr defaultPresenter(mockDefault);
    EXPECT_CALL(*mockDefault, setShown(expectedFlag)).Times(1); // Expect the method on the default to be called.

    // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView, defaultPresenter);
    // Call the method on the composite.
    composite.setShown(boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(), expectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockDefault));
  }

  void test_setBackgroundRadiusShown_default()
  {
    const bool expectedFlag = true;

    // Create a default.
    MockPeaksPresenter* mockDefault = new MockPeaksPresenter;
    PeaksPresenter_sptr defaultPresenter(mockDefault);
    EXPECT_CALL(*mockDefault, showBackgroundRadius(expectedFlag)).Times(1); // Expect the method on the default to be called.

     // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView, defaultPresenter);
    // Call the method on the composite.
    composite.setBackgroundRadiusShown(boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(), expectedFlag);

    TS_ASSERT(Mock::VerifyAndClearExpectations(mockDefault));
  }

  void test_getBackroundColour_default()
  {
    CompositePeaksPresenter composite(&_fakeZoomableView);
    TSM_ASSERT_THROWS("Cannot fetch background colours until nested presenters have been added.", composite.getBackgroundColour(boost::make_shared<Mantid::DataObjects::PeaksWorkspace>()), std::runtime_error&);
  }

  void test_getForegroundColour_default()
  {
    CompositePeaksPresenter composite(&_fakeZoomableView);
    TSM_ASSERT_THROWS("Cannot fetch foreground colours until nested presenters have been added.", composite.getForegroundColour(boost::make_shared<Mantid::DataObjects::PeaksWorkspace>()), std::runtime_error&);
  }

  void test_zoomToPeak()
  {
    const int peakIndex = 0;
    // Prepare subject objects.
    Mantid::API::IPeaksWorkspace_sptr peaksWS = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>(); 
    SetPeaksWorkspaces set;
    set.insert(peaksWS);
    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pSubject, getBoundingBox(peakIndex)).Times(1).WillOnce(Return(PeakBoundingBox()));
    EXPECT_CALL(*pSubject, presentedWorkspaces()).WillOnce(Return(set));

    //Prepare zoomable peak view.
    MockZoomablePeaksView mockZoomableView;
    EXPECT_CALL(mockZoomableView, zoomToRectangle(_)).Times(1);

    // Create the composite and add the test presenter.
    CompositePeaksPresenter composite(&mockZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);

    composite.zoomToPeak(peaksWS, peakIndex);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockZoomableView));
  }

  void test_setPeakSizeOnProjection()
  {
    const double fraction = 0.5;

    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pSubject, setPeakSizeOnProjection(fraction)).Times(1);

    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);
    composite.setPeakSizeOnProjection(fraction);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
  }

  void test_setPeakSizeIntoProjection()
  {
    const double fraction = 0.5;

    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pSubject, setPeakSizeIntoProjection(fraction)).Times(1);

    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);
    composite.setPeakSizeIntoProjection(fraction);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
  }

  void test_getPeakSizeOnProjection_default()
  {
    MockPeaksPresenter* pDefault = new MockPeaksPresenter;
    PeaksPresenter_sptr defaultPresenter(pDefault);
    EXPECT_CALL(*pDefault, getPeakSizeOnProjection()).WillOnce(Return(0));

    CompositePeaksPresenter composite(&_fakeZoomableView, defaultPresenter);
    TS_ASSERT_EQUALS(0, composite.getPeakSizeOnProjection())

    TS_ASSERT(Mock::VerifyAndClearExpectations(pDefault));
  }

  void test_getPeakSizeIntoProjection_default()
  {
    MockPeaksPresenter* pDefault = new MockPeaksPresenter;
    PeaksPresenter_sptr defaultPresenter(pDefault);
    EXPECT_CALL(*pDefault, getPeakSizeIntoProjection()).WillOnce(Return(0));

    CompositePeaksPresenter composite(&_fakeZoomableView, defaultPresenter);
    TS_ASSERT_EQUALS(0, composite.getPeakSizeIntoProjection())

    TS_ASSERT(Mock::VerifyAndClearExpectations(pDefault));
  }

  void test_getPeakSizeOnProjection()
  {
    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pSubject, getPeakSizeOnProjection()).WillOnce(Return(1));

    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);
    TS_ASSERT_EQUALS(1, composite.getPeakSizeOnProjection())

    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
  }

  void test_getPeakSizeIntoProjection()
  {
    MockPeaksPresenter* pSubject = new MockPeaksPresenter;
    PeaksPresenter_sptr subject(pSubject);
    EXPECT_CALL(*pSubject, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pSubject, getPeakSizeIntoProjection()).WillOnce(Return(1));

    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pSubject, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(subject);
    TS_ASSERT_EQUALS(1, composite.getPeakSizeIntoProjection())

    TS_ASSERT(Mock::VerifyAndClearExpectations(pSubject));
  }

  void test_getPeaksPresenter_throws_if_unknown_name()
  {
    CompositePeaksPresenter composite(&_fakeZoomableView);
    TSM_ASSERT_THROWS("Search should fail to find any presenters, as there are none.", composite.getPeaksPresenter(QString("x")), std::invalid_argument&);
  }

  void test_lookup_presenters_via_workspace_names_using_getPeaksPresenter()
  {
    using namespace Mantid::API;

    //One nested presenter. Create setup environment.
    IPeaksWorkspace_sptr peaksWS_1 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
    IPeaksWorkspace_sptr peaksWS_2 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
    AnalysisDataService::Instance().add("ws1", peaksWS_1);
    AnalysisDataService::Instance().add("ws2", peaksWS_2);
    SetPeaksWorkspaces set;
    set.insert(peaksWS_1);
    set.insert(peaksWS_2);
    MockPeaksPresenter* pPresenter = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter(pPresenter);
    EXPECT_CALL(*pPresenter, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pPresenter, presentedWorkspaces()).WillRepeatedly(Return(set));

    /*

      composite
          |
          |
      -----
      |
      p1 (ws1, ws2)

     */

    // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pPresenter, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(presenter);

    // Now perform searches
    PeaksPresenter* foundPresenter = composite.getPeaksPresenter(QString("ws1"));
    TS_ASSERT_EQUALS(foundPresenter, pPresenter)
    foundPresenter = composite.getPeaksPresenter(QString("ws2"));
    TS_ASSERT_EQUALS(foundPresenter, pPresenter)

    // Clean up.
    TS_ASSERT(Mock::VerifyAndClearExpectations(pPresenter));
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void test_lookup_presenters_via_workspace_names_using_getPeaksPresenter_continued()
  {
    using namespace Mantid::API;

    //One nested presenter. Create setup environment.
    IPeaksWorkspace_sptr peaksWS_1 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
    IPeaksWorkspace_sptr peaksWS_2 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
    AnalysisDataService::Instance().add("ws1", peaksWS_1);
    AnalysisDataService::Instance().add("ws2", peaksWS_2);
    SetPeaksWorkspaces set1;
    set1.insert(peaksWS_1);
    SetPeaksWorkspaces set2;
    set2.insert(peaksWS_2);

    MockPeaksPresenter* pPresenter1 = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter1(pPresenter1);

    MockPeaksPresenter* pPresenter2 = new MockPeaksPresenter;
    PeaksPresenter_sptr presenter2(pPresenter2);


    EXPECT_CALL(*pPresenter1, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pPresenter1, presentedWorkspaces()).WillRepeatedly(Return(set1));

    EXPECT_CALL(*pPresenter2, registerOwningPresenter(_)).Times(AtLeast(1));
    EXPECT_CALL(*pPresenter2, presentedWorkspaces()).WillRepeatedly(Return(set2));

    // Create the composite.
    CompositePeaksPresenter composite(&_fakeZoomableView);
    EXPECT_CALL(*pPresenter1, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    EXPECT_CALL(*pPresenter2, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
    composite.addPeaksPresenter(presenter1);
    composite.addPeaksPresenter(presenter2);

    /*

      composite
          |
          |
      -------------
      |            |
      p1 (ws1)     p2 (ws2)


     */

    // Now perform searches
    PeaksPresenter* foundPresenter = composite.getPeaksPresenter(QString("ws1"));
    TS_ASSERT_EQUALS(foundPresenter, pPresenter1)
    foundPresenter = composite.getPeaksPresenter(QString("ws2"));
    TS_ASSERT_EQUALS(foundPresenter, pPresenter2)

    // Clean up.
    TS_ASSERT(Mock::VerifyAndClearExpectations(pPresenter1));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pPresenter2));
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

  void test_notify_workspace_replaced_with_same_ADS_key()
  {
      using namespace Mantid::API;

      //One nested presenter. Create setup environment.
      IPeaksWorkspace_sptr peaksWS_1 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
      IPeaksWorkspace_sptr peaksWS_2 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
      AnalysisDataService::Instance().add("ws1", peaksWS_1);
      AnalysisDataService::Instance().add("ws2", peaksWS_2);
      SetPeaksWorkspaces set1;
      set1.insert(peaksWS_1);
      SetPeaksWorkspaces set2;
      set2.insert(peaksWS_2);

      MockPeaksPresenter* pPresenter1 = new MockPeaksPresenter;
      PeaksPresenter_sptr presenter1(pPresenter1);

      MockPeaksPresenter* pPresenter2 = new MockPeaksPresenter;
      PeaksPresenter_sptr presenter2(pPresenter2);

      EXPECT_CALL(*pPresenter1, registerOwningPresenter(_)).Times(AtLeast(1));
      EXPECT_CALL(*pPresenter1, presentedWorkspaces()).WillRepeatedly(Return(set1));

      EXPECT_CALL(*pPresenter2, registerOwningPresenter(_)).Times(AtLeast(1));
      EXPECT_CALL(*pPresenter2, presentedWorkspaces()).WillRepeatedly(Return(set2));


      // Create the composite.
      CompositePeaksPresenter composite(&_fakeZoomableView);
      EXPECT_CALL(*pPresenter1, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
      EXPECT_CALL(*pPresenter2, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
      composite.addPeaksPresenter(presenter1);
      composite.addPeaksPresenter(presenter2);

      /*

        composite
            |
            |
        -------------
        |            |
        p1 (ws1)     p2 (ws2)


       */

      /*
      Same key different object.
      Now we are going to replace ws2, and we expect the sub presenter for that to be informed.
      */

      EXPECT_CALL(*pPresenter2, reInitialize(_)).Times(1);

      peaksWS_2 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
      AnalysisDataService::Instance().addOrReplace("ws2", peaksWS_2); // Same key (name) different object.
      composite.notifyWorkspaceChanged("ws2", peaksWS_2);

      // Clean up.
      TS_ASSERT(Mock::VerifyAndClearExpectations(pPresenter1));
      TS_ASSERT(Mock::VerifyAndClearExpectations(pPresenter2));
      AnalysisDataService::Instance().remove("ws1");
      AnalysisDataService::Instance().remove("ws2");

  }

  void test_notify_workspace_renamed_in_ADS()
  {
      using namespace Mantid::API;

      //One nested presenter. Create setup environment.
      IPeaksWorkspace_sptr peaksWS_1 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
      IPeaksWorkspace_sptr peaksWS_2 = boost::make_shared<Mantid::DataObjects::PeaksWorkspace>();
      AnalysisDataService::Instance().add("ws1", peaksWS_1);
      AnalysisDataService::Instance().add("ws2", peaksWS_2);
      SetPeaksWorkspaces set1;
      set1.insert(peaksWS_1);
      SetPeaksWorkspaces set2;
      set2.insert(peaksWS_2);

      MockPeaksPresenter* pPresenter1 = new MockPeaksPresenter;
      PeaksPresenter_sptr presenter1(pPresenter1);

      MockPeaksPresenter* pPresenter2 = new MockPeaksPresenter;
      PeaksPresenter_sptr presenter2(pPresenter2);


      EXPECT_CALL(*pPresenter1, registerOwningPresenter(_)).Times(AtLeast(1));
      EXPECT_CALL(*pPresenter1, presentedWorkspaces()).WillRepeatedly(Return(set1));

      EXPECT_CALL(*pPresenter2, registerOwningPresenter(_)).Times(AtLeast(1));
      EXPECT_CALL(*pPresenter2, presentedWorkspaces()).WillRepeatedly(Return(set2));


      // Create the composite.
      CompositePeaksPresenter composite(&_fakeZoomableView);
      EXPECT_CALL(*pPresenter1, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
      EXPECT_CALL(*pPresenter2, contentsDifferent(_)).WillRepeatedly(Return(true)); // Allows us to add to composite
      composite.addPeaksPresenter(presenter1);
      composite.addPeaksPresenter(presenter2);

      /*

        composite
            |
            |
        -------------
        |            |
        p1 (ws1)     p2 (ws2)


       */

      /*
      Same object different key.
      Now we are going to rename ws2, and we expect the sub presenter for that to be informed.

      peaksWS_2 is already a worspace managed by one of the sub-presenters, so that subpresenter should be updated with the new name.
      */
      EXPECT_CALL(*pPresenter2, reInitialize(_)).Times(1);
      AnalysisDataService::Instance().addOrReplace("ws3", peaksWS_2); // Same value (object) different key.
      composite.notifyWorkspaceChanged("ws3", peaksWS_2);


      // Clean up.
      TS_ASSERT(Mock::VerifyAndClearExpectations(pPresenter1));
      TS_ASSERT(Mock::VerifyAndClearExpectations(pPresenter2));
      AnalysisDataService::Instance().remove("ws1");
      AnalysisDataService::Instance().remove("ws2");
      AnalysisDataService::Instance().remove("ws3");

  }



};

#endif
