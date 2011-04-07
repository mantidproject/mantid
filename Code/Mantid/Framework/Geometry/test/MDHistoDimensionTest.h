#ifndef MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_
#define MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

using namespace Mantid::Geometry;

class MDHistoDimensionTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    MDHistoDimension d("name", "id", "Furlongs", -10, 20.0, 15);
    TS_ASSERT_EQUALS(d.getName(), "name");
    TS_ASSERT_EQUALS(d.getDimensionId(), "id");
    TS_ASSERT_EQUALS(d.getUnits(), "Furlongs");
    TS_ASSERT_EQUALS(d.getMinimum(), -10);
    TS_ASSERT_EQUALS(d.getMaximum(), +20);
    TS_ASSERT_EQUALS(d.getNBins(), 15);
    TS_ASSERT_DELTA(d.getBinWidth(), 2.0, 1e-5);
  }

  void test_toXMLStringIntegrated()
  {
    std::string expectedXML =std::string( 
      "<Dimension ID=\"id\">") +
      "<Name>name</Name>" +
      "<UpperBounds>20.0000</UpperBounds>" +
      "<LowerBounds>-10.0000</LowerBounds>" +
      "<NumberOfBins>1</NumberOfBins>" +
      "<Integrated>" +
      "<UpperLimit>20.0000</UpperLimit>" +
      "<LowerLimit>-10.0000</LowerLimit>" +
      "</Integrated>" +
      "</Dimension>";
   
    MDHistoDimension dimension("name", "id", "Furlongs", -10, 20.0, 1); 
    std::string actualXML = dimension.toXMLString();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }

  void test_toXMLStringNotIntegrated()
  {
    std::string expectedXML =std::string( 
      "<Dimension ID=\"id\">") +
      "<Name>name</Name>" +
      "<UpperBounds>20.0000</UpperBounds>" +
      "<LowerBounds>-10.0000</LowerBounds>" +
      "<NumberOfBins>15</NumberOfBins>" +
      "</Dimension>";

    MDHistoDimension dimension("name", "id", "Furlongs", -10, 20.0, 15);
    std::string actualXML = dimension.toXMLString();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }


};


#endif /* MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_ */

