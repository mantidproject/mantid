#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

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
      TS_ASSERT_EQUALS( ws.getSignalAt(i), 0.0);
      TS_ASSERT_EQUALS( ws.getErrorAt(i), 0.0);
    }

    // Setting and getting
    ws.setSignalAt(5,2.3456);
    TS_ASSERT_DELTA( ws.getSignalAt(5), 2.3456, 1e-5);

    ws.setErrorAt(5,1.234);
    TS_ASSERT_DELTA( ws.getErrorAt(5), 1.234, 1e-5);

    std::vector<double> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 5*5*5*5);
    TS_ASSERT_DELTA( data[5], 2.3456, 1e-5);

  }
  
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

