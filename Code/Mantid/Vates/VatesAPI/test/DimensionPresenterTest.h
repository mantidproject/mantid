#ifndef DIMENSION_PRESENTER_TEST_H_
#define DIMENSION_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/DimensionPresenter.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
 
using namespace Mantid;
using namespace Mantid::VATES;
using namespace Mantid::Geometry;
using namespace testing;

class DimensionPresenterTest: public CxxTest::TestSuite
{
private:

  class MockDimensionView : public DimensionView
  {
  public:
    MOCK_METHOD0(configureStrongly, void());
    MOCK_METHOD0(configureWeakly, void());
    MOCK_METHOD1(showAsNotIntegrated, void(VecIMDDimension_sptr));
    MOCK_METHOD0(showAsIntegrated, void());
    MOCK_METHOD1(accept, void(DimensionPresenter*));
    MOCK_CONST_METHOD0(getMinimum, double());
    MOCK_CONST_METHOD0(getMaximum, double());
    MOCK_CONST_METHOD0(getNBins, unsigned int());
    MOCK_CONST_METHOD0(getSelectedIndex, unsigned int());
    MOCK_CONST_METHOD0(getIsIntegrated, bool());
    MOCK_CONST_METHOD0(getVisDimensionName, std::string());
    MOCK_CONST_METHOD1(displayError, void(std::string));
    ~MockDimensionView(){};
  };

  class MockGeometryPresenter : public GeometryPresenter
  {
  public:
    MOCK_METHOD1(dimensionRealigned, void(DimensionPresenter*));
    MOCK_METHOD1(dimensionResized, void(DimensionPresenter*));
    MOCK_CONST_METHOD0(getNonIntegratedDimensions, VecIMDDimension_sptr());
    MOCK_CONST_METHOD0(getGeometryXML, std::string());
    MOCK_METHOD1(acceptView, void(GeometryView*));
    MOCK_METHOD0(setModified, void());
    MOCK_CONST_METHOD0(getMappings, GeometryPresenter::MappingType());
    ~MockGeometryPresenter(){}
  };

    /// Mock IMDDimension allows tests to specify exact expected behavior of dependency.
  class MockIMDDimension : public IMDDimension {
  public:
    MOCK_CONST_METHOD0(getName,
      std::string());
    MOCK_CONST_METHOD0(getUnits,
      std::string());
    MOCK_CONST_METHOD0(getDimensionId,
      std::string());
    MOCK_CONST_METHOD0(getMaximum,
      coord_t());
    MOCK_CONST_METHOD0(getMinimum,
        coord_t());
    MOCK_CONST_METHOD0(getNBins,
      size_t());
    MOCK_CONST_METHOD0(toXMLString,
      std::string());
    MOCK_CONST_METHOD0(getIsIntegrated,
      bool());
    MOCK_CONST_METHOD1(getX,
        coord_t(size_t ind));
    MOCK_METHOD3(setRange,
      void(size_t, coord_t, coord_t));
  };

public:

  void testSetMapping()
  {
    IMDDimension_sptr model(new MockIMDDimension());
    MockDimensionView view;
    MockGeometryPresenter gPresenter;
    DimensionPresenter presenter(&view, &gPresenter);

    TSM_ASSERT("Should have no mapping", presenter.getMapping().empty());

    presenter.setMapping("Z-AXIS");
    TSM_ASSERT_EQUALS("Should now have mapping set", "Z-AXIS", presenter.getMapping());
  }

  void testWithoutProperConstructionThrows()
  {
    IMDDimension_sptr model(new MockIMDDimension());
    MockDimensionView view;
    MockGeometryPresenter gPresenter;
    DimensionPresenter presenter(&view, &gPresenter);

    TSM_ASSERT_THROWS("::acceptModel not called first, so should have thrown", presenter.updateModel(), std::runtime_error);
  }

  void testAcceptModelStrongly()
  {
    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getDimensionId()).Times(2).WillRepeatedly(Return("1"));
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(1);
    IMDDimension_sptr model(pMockDimension);
    
    MockDimensionView view;
    EXPECT_CALL(view, configureStrongly()).Times(1);
    EXPECT_CALL(view, showAsNotIntegrated(_)).Times(1);

    MockGeometryPresenter gPresenter;
    EXPECT_CALL(gPresenter, getNonIntegratedDimensions()).Times(1).WillOnce(Return(VecIMDDimension_sptr()));
    
    DimensionPresenter presenter(&view, &gPresenter);
    presenter.acceptModelStrongly(model);

    TSM_ASSERT_EQUALS("Applied model should be the same as the one provided", model->getDimensionId(), presenter.getModel()->getDimensionId());
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

  void testAcceptModelWeakly()
  {
    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getDimensionId()).Times(2).WillRepeatedly(Return("1"));
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(1);
    IMDDimension_sptr model(pMockDimension);
    
    MockDimensionView view;
    EXPECT_CALL(view, configureWeakly()).Times(1);
    EXPECT_CALL(view, showAsNotIntegrated(_)).Times(1);

    MockGeometryPresenter gPresenter;
    EXPECT_CALL(gPresenter, getNonIntegratedDimensions()).Times(1).WillOnce(Return(VecIMDDimension_sptr()));
    
    DimensionPresenter presenter(&view, &gPresenter);
    presenter.acceptModelWeakly(model);

    TSM_ASSERT_EQUALS("Applied model should be the same as the one provided", model->getDimensionId(), presenter.getModel()->getDimensionId());
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

  void testDriveViewToBeIntegrated()
  {
    MockDimensionView view;
    EXPECT_CALL(view, configureStrongly()).Times(1);
    EXPECT_CALL(view, showAsIntegrated()).Times(2);
    EXPECT_CALL(view, showAsNotIntegrated(_)).Times(0); //Explicitly should never use this.
    EXPECT_CALL(view, getIsIntegrated()).WillOnce(Return(true));
    EXPECT_CALL(view, getVisDimensionName()).Times(1);

    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(1).WillRepeatedly(Return(true)); //Model says it's integrated
    Mantid::Geometry::IMDDimension_sptr model(pMockDimension);

    MockGeometryPresenter gPresenter;
    EXPECT_CALL(gPresenter, setModified()).Times(1);

    DimensionPresenter presenter(&view, &gPresenter); 
    presenter.acceptModelStrongly(model);
    TSM_ASSERT_THROWS_NOTHING("A model exists on the presenter, updating should it should operate without exception.", presenter.updateModel());


    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

  void testDriveViewToBeNotIntegrated()
  {
    MockDimensionView view;
    EXPECT_CALL(view, configureStrongly()).Times(1);
    EXPECT_CALL(view, showAsNotIntegrated(_)).Times(2);
    EXPECT_CALL(view, showAsIntegrated()).Times(0); //Explicitly should never use this
    EXPECT_CALL(view, getIsIntegrated()).Times(AnyNumber()).WillRepeatedly(Return(false));//View is not integrated. 
    EXPECT_CALL(view, getVisDimensionName()).Times(1); 

    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(AnyNumber()).WillRepeatedly(Return(false));
    Mantid::Geometry::IMDDimension_sptr model(pMockDimension); 

    MockGeometryPresenter gPresenter;
    EXPECT_CALL(gPresenter, getNonIntegratedDimensions()).Times(2).WillRepeatedly(Return(VecIMDDimension_sptr())) ; //Will ask the GeometryPresenter for non-integrated dimensions
    EXPECT_CALL(gPresenter, setModified()).Times(1);
    DimensionPresenter presenter(&view, &gPresenter);
    presenter.acceptModelStrongly(model);
    
    TSM_ASSERT_THROWS_NOTHING("A model exists on the presenter, updating should it should operate without exception.", presenter.updateModel());
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

  void testGetAppliedModelWhenViewIntegrated()
  {
    MockDimensionView view;
    EXPECT_CALL(view, configureStrongly()).Times(1);
    EXPECT_CALL(view, showAsIntegrated()).Times(1);
    EXPECT_CALL(view, getIsIntegrated()).Times(1).WillRepeatedly(Return(true)); // view says it's integrated
    EXPECT_CALL(view, getMinimum()).Times(1).WillOnce(Return(0));
    EXPECT_CALL(view, getMaximum()).Times(1).WillOnce(Return(2));
    EXPECT_CALL(view, getNBins()).Times(0); //Should never need number of bins because view says it's integrated.

    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(1).WillRepeatedly(Return(true)); //Model says it's integrated
    EXPECT_CALL(*pMockDimension, toXMLString()).WillOnce(Return("<Dimension ID=\"en\"><Name>Energy</Name><UpperBounds>150</UpperBounds><LowerBounds>0</LowerBounds><NumberOfBins>1</NumberOfBins></Dimension>"));
    Mantid::Geometry::IMDDimension_sptr model(pMockDimension);

    MockGeometryPresenter gPresenter;

    DimensionPresenter presenter(&view, &gPresenter); 
    presenter.acceptModelStrongly(model);
    Mantid::Geometry::IMDDimension_sptr product = presenter.getAppliedModel();

    TSM_ASSERT_EQUALS("Wrong number of bins for an integrated dimension", 1, product->getNBins());
    TSM_ASSERT_EQUALS("Range max not set properly", 2, product->getMaximum());
    TSM_ASSERT_EQUALS("Range min not set properly", 0, product->getMinimum());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

  void testHandleArgumentErrors()
  {
    MockDimensionView view;
    EXPECT_CALL(view, configureStrongly()).Times(AnyNumber());
    EXPECT_CALL(view, showAsIntegrated()).Times(AnyNumber());
    EXPECT_CALL(view, getIsIntegrated()).Times(AtLeast(1)).WillRepeatedly(Return(false)); // view says it's integrated
    EXPECT_CALL(view, getMinimum()).Times(AnyNumber()).WillRepeatedly(Return(10)); //Ooops, min > max, this should be handled! 
    EXPECT_CALL(view, getMaximum()).Times(AnyNumber()).WillRepeatedly(Return(2));
    EXPECT_CALL(view, getNBins()).Times(AnyNumber()); 
    EXPECT_CALL(view, displayError(_)).Times(1);

    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(AnyNumber()).WillRepeatedly(Return(true)); //Model says it's integrated
    EXPECT_CALL(*pMockDimension, toXMLString()).Times(AnyNumber()).WillRepeatedly(Return("<Dimension ID=\"en\"><Name>Energy</Name><UpperBounds>150</UpperBounds><LowerBounds>0</LowerBounds><NumberOfBins>1</NumberOfBins></Dimension>"));
    Mantid::Geometry::IMDDimension_sptr model(pMockDimension);

    MockGeometryPresenter gPresenter;

    DimensionPresenter presenter(&view, &gPresenter); 
    presenter.acceptModelStrongly(model);
    presenter.getAppliedModel();

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

};

#endif
