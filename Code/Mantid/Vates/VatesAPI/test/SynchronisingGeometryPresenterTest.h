#ifndef SYNCHRONISING_GEOMETRY_PRESENTER_TEST_H_
#define SYNCHRONISING_GEOMETRY_PRESENTER_TEST_H_ 

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/SynchronisingGeometryPresenter.h"
#include "MantidVatesAPI/GeometryView.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidVatesAPI/GeometryXMLParser.h"
#include "MantidVatesAPI/DimensionPresenter.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace testing;
using namespace Mantid::VATES;
using namespace Mantid::Geometry;

class SyncronisingGeometryPresenterTest: public CxxTest::TestSuite
{
  

private:

static std::string constructXML()
{
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
  "<DimensionSet>" +
    "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<UpperBounds>150</UpperBounds>" +
      "<LowerBounds>0</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" +
      "<UpperBounds>5</UpperBounds>" +
      "<LowerBounds>-1.5</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qy\">" +
      "<Name>Qy</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qz\">" +
      "<Name>Qz</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"other\">" +
      "<Name>Other</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
    "</Dimension>" +
    "<XDimension>" +
      "<RefDimensionId>qx</RefDimensionId>" +
    "</XDimension>" +
    "<YDimension>" +
      "<RefDimensionId>qy</RefDimensionId>" +
    "</YDimension>" +
    "<ZDimension>" +
      "<RefDimensionId>qz</RefDimensionId>" +
    "</ZDimension>" +
    "<TDimension>" +
      "<RefDimensionId>en</RefDimensionId>" +
    "</TDimension>" +
  "</DimensionSet>";
  }

  class MockGeometryView : public GeometryView
  {
  public:
    MOCK_METHOD1(addDimensionView, void(DimensionView*));
    MOCK_CONST_METHOD0(getGeometryXMLString, std::string());
    MOCK_METHOD0(getDimensionViewFactory, const DimensionViewFactory&());
    MOCK_METHOD0(raiseModified, void());
    MOCK_METHOD0(raiseNoClipping, void());
    ~MockGeometryView(){}
  };

  class MockDimensionViewFactory : public DimensionViewFactory
  {
  public:
    MOCK_CONST_METHOD0( create, DimensionView*());
    ~MockDimensionViewFactory(){}
  };

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

  class ExposedSynchronisingGeometryPresenter : public Mantid::VATES::SynchronisingGeometryPresenter
  {
  public:
    ExposedSynchronisingGeometryPresenter(Mantid::VATES::GeometryXMLParser& source) : Mantid::VATES::SynchronisingGeometryPresenter(source) {}
    Mantid::VATES::DimPresenter_sptr getDimensionPresenter(unsigned int index)
    {
      return m_dimPresenters[index];
    }
  };

public:

  void testConstruct()
  {
    GeometryXMLParser parser(constructXML());
    parser.execute();
    SynchronisingGeometryPresenter presenter(parser); 
    TSM_ASSERT_EQUALS("Wrong number of nonintegrated dimensions", 3, presenter.getNonIntegratedDimensions().size());
  }

  void testAcceptView()
  {
    MockDimensionView dView;
    EXPECT_CALL(dView, accept(_)).Times(5);
    EXPECT_CALL(dView, configure()).Times(5);
    EXPECT_CALL(dView, showAsNotIntegrated(_)).Times(3);
    EXPECT_CALL(dView, showAsIntegrated()).Times(2);

    MockDimensionViewFactory factory;
    EXPECT_CALL(factory, create()).Times(5).WillRepeatedly(Return(&dView));
    
    MockGeometryView gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).Times(1).WillRepeatedly(ReturnRef(factory));
    EXPECT_CALL(gView, addDimensionView(_)).Times(5);

    GeometryXMLParser parser(constructXML());
    parser.execute();

    SynchronisingGeometryPresenter presenter(parser); 
    presenter.acceptView(&gView);
    
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testGetGeometryXML()
  {
    NiceMock<MockDimensionView> dView;
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));

    GeometryXMLParser parser(constructXML());
    parser.execute();

    SynchronisingGeometryPresenter presenter(parser); 
    presenter.acceptView(&gView);

    TSM_ASSERT("Geometry XML has not been constructed", !presenter.getGeometryXML().empty()); 
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testGetLabels()
  {
    NiceMock<MockDimensionView> dView;
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));

    GeometryXMLParser parser(constructXML());
    parser.execute();

    ExposedSynchronisingGeometryPresenter presenter(parser); 
    presenter.acceptView(&gView);

    DimPresenter_sptr xPresenter = presenter.getDimensionPresenter(0);
    DimPresenter_sptr yPresenter = presenter.getDimensionPresenter(1);
    DimPresenter_sptr zPresenter = presenter.getDimensionPresenter(2);
    DimPresenter_sptr tPresenter = presenter.getDimensionPresenter(3);

    TSM_ASSERT_EQUALS("Fetched label is not correct.", "T Axis" ,presenter.getLabel(xPresenter.get())); 
    TSM_ASSERT_EQUALS("Fetched label is not correct.", "X Axis" ,presenter.getLabel(yPresenter.get())); 
    TSM_ASSERT_EQUALS("Fetched label is not correct.", "Y Axis" ,presenter.getLabel(zPresenter.get())); 
    TSM_ASSERT_EQUALS("Fetched label is not correct.", "Z Axis" ,presenter.getLabel(tPresenter.get())); 

    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testDimensionRealign()
  {
    NiceMock<MockDimensionView> dView;
    EXPECT_CALL(dView, getDimensionId()).WillRepeatedly(Return("qx"));
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));

    GeometryXMLParser parser(constructXML());
    parser.execute();

    ExposedSynchronisingGeometryPresenter presenter(parser); 
    presenter.acceptView(&gView);

    DimPresenter_sptr presenterA = presenter.getDimensionPresenter(0);
    DimPresenter_sptr presenterB = presenter.getDimensionPresenter(1);

    TS_ASSERT_EQUALS("en" , presenterA->getModel()->getDimensionId()); 
    TS_ASSERT_EQUALS("qx" , presenterB->getModel()->getDimensionId()); 

    presenter.dimensionRealigned(presenterA.get()); //Now swap these two dimensions

    TS_ASSERT_EQUALS("qx" , presenterA->getModel()->getDimensionId());  
    TS_ASSERT_EQUALS("en" , presenterB->getModel()->getDimensionId()); 

    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

};

#endif