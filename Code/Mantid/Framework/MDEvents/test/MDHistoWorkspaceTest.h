#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <iomanip>
#include <boost/math/special_functions/fpclassify.hpp>

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;

class MDHistoWorkspaceTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 5));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 5));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    TS_ASSERT_EQUALS( ws.getNumDims(), 4);
    TS_ASSERT_EQUALS( ws.getNPoints(), 5*5*5*5);
    TS_ASSERT_EQUALS( ws.getMemorySize(), 5*5*5*5 * sizeof(double)*2);
    TS_ASSERT_EQUALS( ws.getXDimension(), dimX);
    TS_ASSERT_EQUALS( ws.getYDimension(), dimY);
    TS_ASSERT_EQUALS( ws.getZDimension(), dimZ);
    TS_ASSERT_EQUALS( ws.getTDimension(), dimT);

    // Methods that are not implemented
    TS_ASSERT_THROWS_ANYTHING( ws.getCell(1) );
    TS_ASSERT_THROWS_ANYTHING( ws.getCell(1,2,3,4) );
    TS_ASSERT_THROWS_ANYTHING( ws.getPoint(1) );

    // The values are cleared at the start
    for (size_t i=0; i <  ws.getNPoints(); i++)
    {
      TS_ASSERT( boost::math::isnan( ws.getSignalAt(i) ));
      TS_ASSERT( boost::math::isnan( ws.getErrorAt(i) ));
      TS_ASSERT( boost::math::isnan( ws.getSignalNormalizedAt(i) ));
      TS_ASSERT( boost::math::isnan( ws.getErrorNormalizedAt(i) ));
    }

    // Setting and getting
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);
    TS_ASSERT_DELTA( ws.getSignalNormalizedAt(5), 2.3456 / 256.0, 1e-5); // Cell volume is 256

    ws.setErrorAt(5,1.234);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);
    TS_ASSERT_DELTA( ws.getErrorNormalizedAt(5), 1.234 / 256.0, 1e-5); // Cell volume is 256

    std::vector<double> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 5*5*5*5);
    TS_ASSERT_DELTA( data[5], 2.3456, 1e-5);
  }


  //---------------------------------------------------------------------------------------------------
  /** Create a dense histogram with only 2 dimensions */
  void test_constructor_fewerDimensions()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY);

    TS_ASSERT_EQUALS( ws.getNumDims(), 2);
    TS_ASSERT_EQUALS( ws.getNPoints(), 5*5);
    TS_ASSERT_EQUALS( ws.getMemorySize(), 5*5 * sizeof(double)*2);
    TS_ASSERT_EQUALS( ws.getXDimension(), dimX);
    TS_ASSERT_EQUALS( ws.getYDimension(), dimY);
    TS_ASSERT_THROWS_ANYTHING( ws.getZDimension());
    TS_ASSERT_THROWS_ANYTHING( ws.getTDimension());

    // Setting and getting
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);

    ws.setErrorAt(5,1.234);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);

    std::vector<double> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 5*5);
    TS_ASSERT_DELTA( data[5], 2.3456, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Create a dense histogram with 7 dimensions */
  void test_constructor_MoreThanFourDimensions()
  {
    std::vector<MDHistoDimension_sptr> dimensions;
    for (size_t i=0; i<7; i++)
    {
      dimensions.push_back(MDHistoDimension_sptr(new MDHistoDimension("Dim", "Dim", "m", -10, 10, 3)));
    }

    MDHistoWorkspace ws(dimensions);

    TS_ASSERT_EQUALS( ws.getNumDims(), 7);
    TS_ASSERT_EQUALS( ws.getNPoints(), 3*3*3*3*3*3*3);
    TS_ASSERT_EQUALS( ws.getMemorySize(), ws.getNPoints() * sizeof(double)*2);

    // Setting and getting
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);

    ws.setErrorAt(5,1.234);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);

    std::vector<double> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 3*3*3*3*3*3*3);
    TS_ASSERT_DELTA( data[5], 2.3456, 1e-5);
  }



  //---------------------------------------------------------------------------------------------------
  /** Test for a possible seg-fault if nx != ny etc. */
  void test_uneven_numbers_of_bins()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 10));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 20));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 10));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    TS_ASSERT_EQUALS( ws.getNumDims(), 4);
    TS_ASSERT_EQUALS( ws.getNPoints(), 5*10*20*10);
    TS_ASSERT_EQUALS( ws.getMemorySize(), 5*10*20*10 * sizeof(double)*2);

    // Setting and getting
    size_t index = 5*10*20*10-1; // The last point
    ws.setSignalAt(index,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(index), 2.3456, 1e-5);

    // Getter with all indices
    TS_ASSERT_DELTA( ws.getSignalAt(4,9,19,9), 2.3456, 1e-5);

  }

    //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetNonIntegratedDimensions()
  {
    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 1)); //Integrated.
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 10));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 20));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 10));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);
    Mantid::Geometry::VecIMDDimension_const_sptr vecNonIntegratedDims = ws.getNonIntegratedDimensions();
    TSM_ASSERT_EQUALS("Only 3 of the 4 dimensions should be non-integrated", 3, vecNonIntegratedDims.size());
    TSM_ASSERT_EQUALS("First non-integrated dimension should be Y", "y", vecNonIntegratedDims[0]->getDimensionId());
    TSM_ASSERT_EQUALS("Second non-integrated dimension should be Z", "z", vecNonIntegratedDims[1]->getDimensionId());
    TSM_ASSERT_EQUALS("Third non-integrated dimension should be T", "t", vecNonIntegratedDims[2]->getDimensionId());
  }


  //---------------------------------------------------------------------------------------------------
  void test_getGeometryXML()
  {
    //If POCO xml supported schema validation, we wouldn't need to check xml outputs like this.
    std::string expectedXML = std::string("<DimensionSet>") +
      "<Dimension ID=\"x\">" +
      "<Name>X</Name>" + 
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<Dimension ID=\"y\">" +
      "<Name>Y</Name>" + 
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<Dimension ID=\"z\">" +
      "<Name>Z</Name>" + 
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<Dimension ID=\"t\">" +
      "<Name>T</Name>" + 
      "<UpperBounds>10.0000</UpperBounds>" + 
      "<LowerBounds>-10.0000</LowerBounds>" + 
      "<NumberOfBins>5</NumberOfBins>" + 
      "</Dimension>" +
      "<XDimension>" +
      "<RefDimensionId>x</RefDimensionId>" +
      "</XDimension>" +
      "<YDimension>" +
      "<RefDimensionId>y</RefDimensionId>" + 
      "</YDimension>" +
      "<ZDimension>" +
      "<RefDimensionId>z</RefDimensionId>" + 
      "</ZDimension>" +
      "<TDimension>" +
      "<RefDimensionId>t</RefDimensionId>" + 
      "</TDimension>" +
      "</DimensionSet>";

    MDHistoDimension_sptr dimX(new MDHistoDimension("X", "x", "m", -10, 10, 5));
    MDHistoDimension_sptr dimY(new MDHistoDimension("Y", "y", "m", -10, 10, 5));
    MDHistoDimension_sptr dimZ(new MDHistoDimension("Z", "z", "m", -10, 10, 5));
    MDHistoDimension_sptr dimT(new MDHistoDimension("T", "t", "m", -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    std::string actualXML = ws.getGeometryXML();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }


};


#endif /* MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_ */

