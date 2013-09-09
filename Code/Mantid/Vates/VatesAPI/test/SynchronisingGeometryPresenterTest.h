#ifndef SYNCHRONISING_GEOMETRY_PRESENTER_TEST_H_
#define SYNCHRONISING_GEOMETRY_PRESENTER_TEST_H_ 

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/SynchronisingGeometryPresenter.h"
#include "MantidVatesAPI/GeometryView.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLParser.h"
#include "MantidVatesAPI/DimensionPresenter.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace testing;
using namespace Mantid::VATES;
using namespace Mantid::Geometry;

class SynchronisingGeometryPresenterTest: public CxxTest::TestSuite
{
  

private:

static std::string constructXML(std::string nbinsA, std::string nbinsB, std::string nbinsC, std::string nbinsD, std::string nbinsE)
{
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
  "<DimensionSet>" +
    "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<UpperBounds>150</UpperBounds>" +
      "<LowerBounds>0</LowerBounds>" +
      "<NumberOfBins>" + nbinsA + "</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" +
      "<UpperBounds>5</UpperBounds>" +
      "<LowerBounds>-1.5</LowerBounds>" +
      "<NumberOfBins>" + nbinsB + "</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qy\">" +
      "<Name>Qy</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>" + nbinsC + "</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qz\">" +
      "<Name>Qz</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>" + nbinsD + "</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"other\">" +
      "<Name>Other</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>" + nbinsE + "</NumberOfBins>" +
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

  static std::string constructXML()
  {
    return constructXML("1", "5", "5", "5", "3");
  }

  class MockGeometryView : public GeometryView
  {
  public:
    MOCK_METHOD1(addDimensionView, void(DimensionView*));
    MOCK_CONST_METHOD0(getGeometryXMLString, std::string());
    MOCK_METHOD0(getDimensionViewFactory, const DimensionViewFactory&());
    MOCK_METHOD0(raiseModified, void());
    MOCK_METHOD0(raiseNoClipping, void());
    MOCK_CONST_METHOD0(getBinDisplayMode, BinDisplay());
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
    MOCK_METHOD0(configureStrongly, void());
    MOCK_METHOD0(configureWeakly, void());
    MOCK_METHOD1(showAsNotIntegrated, void(VecIMDDimension_sptr));
    MOCK_METHOD0(showAsIntegrated, void());
    MOCK_METHOD1(accept, void(DimensionPresenter*));
    MOCK_CONST_METHOD0(getVisDimensionName, std::string());
    MOCK_CONST_METHOD0(getMinimum, double());
    MOCK_CONST_METHOD0(getMaximum, double());
    MOCK_CONST_METHOD0(getNBins, unsigned int());
    MOCK_CONST_METHOD0(getSelectedIndex, unsigned int());
    MOCK_CONST_METHOD0(getIsIntegrated, bool());
    MOCK_CONST_METHOD1(displayError, void(std::string));
    MOCK_METHOD1(setViewMode, void(Mantid::VATES::BinDisplay));
    ~MockDimensionView(){};
  };

  class ExposedSynchronisingGeometryPresenter : public Mantid::VATES::SynchronisingGeometryPresenter
  {
  public:
    ExposedSynchronisingGeometryPresenter(Mantid::Geometry::MDGeometryXMLParser& source) : Mantid::VATES::SynchronisingGeometryPresenter(source) {}
    Mantid::VATES::DimPresenter_sptr getDimensionPresenter(unsigned int index)
    {
      return m_dimPresenters[index];
    }
  
    void dimensionCollapsed(DimensionPresenter* pDimensionPresenter)
    {
      return Mantid::VATES::SynchronisingGeometryPresenter::dimensionCollapsed(pDimensionPresenter);
    }
  };

public:

  void testConstruct()
  {
    MDGeometryXMLParser parser(constructXML());
    parser.execute();
    SynchronisingGeometryPresenter* pPresenter = NULL;
    TS_ASSERT_THROWS_NOTHING(pPresenter = new SynchronisingGeometryPresenter(parser)); 
    delete pPresenter;
  }

  void testAcceptView()
  {
    MockDimensionView dView;
    EXPECT_CALL(dView, accept(_)).Times(5);
    EXPECT_CALL(dView, configureStrongly()).Times(5);
    EXPECT_CALL(dView, showAsNotIntegrated(_)).Times(4);
    EXPECT_CALL(dView, showAsIntegrated()).Times(1);

    MockDimensionViewFactory factory;
    EXPECT_CALL(factory, create()).Times(5).WillRepeatedly(Return(&dView));
    
    MockGeometryView gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillOnce(ReturnRef(factory));
    EXPECT_CALL(gView, addDimensionView(_)).Times(5);
    EXPECT_CALL(gView, getBinDisplayMode()).Times(1).WillOnce(Return(Simple));

    MDGeometryXMLParser parser(constructXML());
    parser.execute();

    SynchronisingGeometryPresenter presenter(parser); 
    presenter.acceptView(&gView);
    GeometryPresenter::MappingType axisMappings = presenter.getMappings();

    TSM_ASSERT_EQUALS("Wrong number of axis-mappings", 4, axisMappings.size());
    TSM_ASSERT("Doesn't contain x-axis mapping", axisMappings.find(presenter.X_AXIS) != axisMappings.end());
    TSM_ASSERT("Doesn't contain y-axis mapping", axisMappings.find(presenter.Y_AXIS) != axisMappings.end());
    TSM_ASSERT("Doesn't contain z-axis mapping", axisMappings.find(presenter.Z_AXIS) != axisMappings.end());
    TSM_ASSERT("Doesn't contain t-axis mapping", axisMappings.find(presenter.T_AXIS) != axisMappings.end());
    
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));

  }

  void testDimensionPartitioning()
  {
    MDGeometryXMLParser parser(constructXML());
    parser.execute();
    SynchronisingGeometryPresenter presenter(parser);

    VecIMDDimension_sptr nonIntegratedDimensions = presenter.getNonIntegratedDimensions();
    VecIMDDimension_sptr integratedDimensions = presenter.getIntegratedDimensions();

    TSM_ASSERT_EQUALS("Sum of partitions doesn't compute to total", 5, nonIntegratedDimensions.size() + integratedDimensions.size());
    TSM_ASSERT_EQUALS("Wrong number of non-integrated dimensions", 4, nonIntegratedDimensions.size());
    TSM_ASSERT_EQUALS("Wrong number of integrated dimensions", 1, integratedDimensions.size());
    TSM_ASSERT_EQUALS("Wrong integrated dimension", "en", integratedDimensions[0]->getDimensionId());
  }

  void testCollapsingThrows()
  {
    //In this test schenario there is a only one non-integrated dimension.
    MDGeometryXMLParser parser(constructXML("2", "1", "1", "1", "1"));
    parser.execute();
    ExposedSynchronisingGeometryPresenter geometryPresenter(parser);

    MockDimensionView dView;
    DimensionPresenter dimensionPresenter(&dView, &geometryPresenter);
    
    //Should not be able to make a collapse request to the geometry presenter, when there is only one non-collapsed dimension.
    TSM_ASSERT_THROWS("Should not be able to collapse the only-exising non-collapsed dimension.", geometryPresenter.dimensionCollapsed(&dimensionPresenter), std::invalid_argument);
  }

  void testGetGeometryXML()
  {
    NiceMock<MockDimensionView> dView;
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));
    EXPECT_CALL(gView, getBinDisplayMode()).Times(1).WillOnce(Return(Simple));

    MDGeometryXMLParser parser(constructXML());
    parser.execute();

    SynchronisingGeometryPresenter presenter(parser); 
    presenter.acceptView(&gView);

    TSM_ASSERT("Geometry XML has not been constructed", !presenter.getGeometryXML().empty()); 
    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testDimensionRealign()
  {
    NiceMock<MockDimensionView> dView;
    EXPECT_CALL(dView, getVisDimensionName()).WillRepeatedly(Return("T-AXIS"));
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));
    EXPECT_CALL(gView, getBinDisplayMode()).Times(1).WillOnce(Return(Simple));

    MDGeometryXMLParser parser(constructXML());
    parser.execute();

    ExposedSynchronisingGeometryPresenter presenter(parser); 
    presenter.acceptView(&gView);

    //find out what presenter X_DIMENSION maps to.
    DimPresenter_sptr presenterA = presenter.getMappings().at(presenter.X_AXIS);
    DimPresenter_sptr presenterB = presenter.getMappings().at(presenter.T_AXIS); 

    TSM_ASSERT_EQUALS("Swapping has not occured as expected.", presenter.X_AXIS, presenterA->getMapping());
    TSM_ASSERT_EQUALS("Swapping has not occured as expected.", presenter.T_AXIS, presenterB->getMapping());

    presenter.dimensionRealigned(presenterA.get()); //Now swap these two dimensions

    TSM_ASSERT_EQUALS("Swapping has not occured as expected.", presenter.T_AXIS, presenterA->getMapping());
    TSM_ASSERT_EQUALS("Swapping has not occured as expected.", presenter.X_AXIS, presenterB->getMapping());

    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testNoDimensionModeChanged()
  {
    NiceMock<MockDimensionView> dView;
    EXPECT_CALL(dView, setViewMode(_)).Times(0); // Call 0 times since nothing has changed.
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));
    EXPECT_CALL(gView, getBinDisplayMode()).Times(2).WillRepeatedly(Return(Simple)); //Will return (SIMPLE) the same Mode as the original, so nothing should happen.

    MDGeometryXMLParser parser(constructXML());
    parser.execute();

    SynchronisingGeometryPresenter presenter(parser); //Default initalizer sets the mode to SIMPLE
    presenter.acceptView(&gView);

    //Some external indication that the mode has changed.
    presenter.setDimensionModeChanged();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testDimensionModeChangedOnce()
  {
    NiceMock<MockDimensionView> dView;
    EXPECT_CALL(dView, setViewMode(_)).Times(5); // Call 5 times since 5 dimensions are in the xml.
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));
    EXPECT_CALL(gView, getBinDisplayMode()).Times(2).WillOnce(Return(Simple)).WillOnce(Return(LowHighStep)); //Will return (LowHighStep) a different Mode to the original.

    MDGeometryXMLParser parser(constructXML());
    parser.execute();

    SynchronisingGeometryPresenter presenter(parser); //Default initalizer sets the mode to SIMPLE
    presenter.acceptView(&gView);

    //Some external indication that the mode has changed.
    presenter.setDimensionModeChanged();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testDimensionModeChangedDuplicated()
  {
    NiceMock<MockDimensionView> dView;
    EXPECT_CALL(dView, setViewMode(_)).Times(5); // Call 5 times since 5 dimensions are in the xml.
    
    NiceMock<MockDimensionViewFactory> factory;
    EXPECT_CALL(factory, create()).WillRepeatedly(Return(&dView));
    
    NiceMock<MockGeometryView> gView;
    EXPECT_CALL(gView, getDimensionViewFactory()).WillRepeatedly(ReturnRef(factory));
    EXPECT_CALL(gView, getBinDisplayMode()).Times(3).WillOnce(Return(Simple)).WillRepeatedly(Return(LowHighStep)); //Will return (LowHighStep) a different Mode to the original.

    MDGeometryXMLParser parser(constructXML());
    parser.execute();

    SynchronisingGeometryPresenter presenter(parser); //Default initalizer sets the mode to SIMPLE
    presenter.acceptView(&gView);

    //Some external indication that the mode has changed.
    presenter.setDimensionModeChanged();
    presenter.setDimensionModeChanged(); //Calling it again should do nothing because the last result should be cached.

    TS_ASSERT(Mock::VerifyAndClearExpectations(&gView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&dView));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

};

#endif
