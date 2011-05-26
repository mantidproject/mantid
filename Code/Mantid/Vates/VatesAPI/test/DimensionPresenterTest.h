#ifndef DIMENSION_PRESENTER_TEST_H_
#define DIMENSION_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/DimensionPresenter.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include "MantidVatesAPI/DimensionView.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
 
using namespace Mantid::VATES;
using namespace Mantid::Geometry;
using namespace testing;

class DimensionPresenterTest: public CxxTest::TestSuite
{
private:

  class MockDimensionView : public DimensionView
  {
  public:
    MOCK_METHOD0(configure, void());
    MOCK_METHOD1(showAsNotIntegrated, void(VecIMDDimension_sptr));
    MOCK_METHOD0(showAsIntegrated, void());
    MOCK_METHOD1(accept, void(DimensionPresenter*));
    MOCK_CONST_METHOD0(getDimensionId, std::string());
    MOCK_CONST_METHOD0(getMinimum, double());
    MOCK_CONST_METHOD0(getMaximum, double());
    MOCK_CONST_METHOD0(getNBins, unsigned int());
    MOCK_CONST_METHOD0(getSelectedIndex, unsigned int());
    MOCK_CONST_METHOD0(getIsIntegrated, bool());
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
    MOCK_CONST_METHOD1(getLabel, std::string(DimensionPresenter const * const));
    MOCK_METHOD0(setModified, void());
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
      double());
    MOCK_CONST_METHOD0(getMinimum,
      double());
    MOCK_CONST_METHOD0(getNBins,
      size_t());
    MOCK_CONST_METHOD0(toXMLString,
      std::string());
    MOCK_CONST_METHOD0(getIsIntegrated,
      bool());
    MOCK_CONST_METHOD1(getX,
      double(size_t ind));
  };

public:

  void testWithoutProperConstructionThrows()
  {
    IMDDimension_sptr model(new MockIMDDimension());
    MockDimensionView view;
    MockGeometryPresenter gPresenter;
    DimensionPresenter presenter(&view, &gPresenter);

    TSM_ASSERT_THROWS("::acceptModel not called first, so should have thrown", presenter.updateModel(), std::runtime_error, "");
  }

  void testAcceptModel()
  {
    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getDimensionId()).Times(2).WillRepeatedly(Return("1"));
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(1);
    IMDDimension_sptr model(pMockDimension);
    
    MockDimensionView view;
    EXPECT_CALL(view, configure()).Times(1);
    EXPECT_CALL(view, showAsNotIntegrated(_)).Times(1);

    MockGeometryPresenter gPresenter;
    EXPECT_CALL(gPresenter, getNonIntegratedDimensions()).Times(1).WillOnce(Return(VecIMDDimension_sptr()));
    
    DimensionPresenter presenter(&view, &gPresenter);
    presenter.acceptModel(model);

    TSM_ASSERT_EQUALS("Applied model should be the same as the one provided", model->getDimensionId(), presenter.getModel()->getDimensionId());
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

  void testDriveViewToBeIntegrated()
  {
    MockDimensionView view;
    EXPECT_CALL(view, configure()).Times(1);
    EXPECT_CALL(view, showAsIntegrated()).Times(2);
    EXPECT_CALL(view, getIsIntegrated()).WillOnce(Return(true));
    EXPECT_CALL(view, getDimensionId()).Times(1);

    MockIMDDimension* pMockDimension = new MockIMDDimension();
    EXPECT_CALL(*pMockDimension, getIsIntegrated()).Times(1).WillRepeatedly(Return(true)); //Model says it's integrated
    EXPECT_CALL(*pMockDimension, getDimensionId()).Times(1);
    Mantid::Geometry::IMDDimension_sptr model(pMockDimension);

    MockGeometryPresenter gPresenter;
    EXPECT_CALL(gPresenter, setModified()).Times(1);

    DimensionPresenter presenter(&view, &gPresenter); 
    presenter.acceptModel(model);
    
    TSM_ASSERT_THROWS_NOTHING("A model exists on the presenter, updating should it should operate without exception.", presenter.updateModel());
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockDimension));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gPresenter));
  }

  //TODO more tests requried.

};

#endif