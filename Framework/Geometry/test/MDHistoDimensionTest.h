#ifndef MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_
#define MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Geometry;

class MDHistoDimensionTest : public CxxTest::TestSuite {
public:
  void test_constructor_throws() {
    coord_t min = 10;
    coord_t max = 1; // min > max !
    TSM_ASSERT_THROWS("Should throw if min > max!",
                      MDHistoDimension("name", "id", "Furlongs", min, max, 15),
                      std::invalid_argument);
  }

  void test_constructor() {
    MDHistoDimension d("name", "id", "Furlongs", -10, 20.0, 15);
    TS_ASSERT_EQUALS(d.getName(), "name");
    TS_ASSERT_EQUALS(d.getDimensionId(), "id");
    TS_ASSERT_EQUALS(d.getUnits(), "Furlongs");
    TS_ASSERT_EQUALS(d.getMinimum(), -10);
    TS_ASSERT_EQUALS(d.getMaximum(), +20);
    TS_ASSERT_EQUALS(d.getNBins(), 15);
    TS_ASSERT_DELTA(d.getBinWidth(), 2.0, 1e-5);
  }

  void test_toXMLStringIntegrated() {
    std::string expectedXML =
        std::string("<Dimension ID=\"id\">") + "<Name>name</Name>" +
        "<Units>Furlongs</Units>" + "<Frame>Unknown frame</Frame>" +
        "<UpperBounds>20.0000</UpperBounds>" +
        "<LowerBounds>-10.0000</LowerBounds>" +
        "<NumberOfBins>1</NumberOfBins>" + "<Integrated>" +
        "<UpperLimit>20.0000</UpperLimit>" +
        "<LowerLimit>-10.0000</LowerLimit>" + "</Integrated>" + "</Dimension>";

    MDHistoDimension dimension("name", "id", "Furlongs", -10, 20.0, 1);
    std::string actualXML = dimension.toXMLString();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }

  void test_toXMLStringNotIntegrated() {
    std::string expectedXML =
        std::string("<Dimension ID=\"id\">") + "<Name>name</Name>" +
        "<Units>Furlongs</Units>" + "<Frame>Unknown frame</Frame>" +
        "<UpperBounds>20.0000</UpperBounds>" +
        "<LowerBounds>-10.0000</LowerBounds>" +
        "<NumberOfBins>15</NumberOfBins>" + "</Dimension>";

    MDHistoDimension dimension("name", "id", "Furlongs", -10, 20.0, 15);
    std::string actualXML = dimension.toXMLString();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }

  void test_getMDUnits_gives_label_unit() {

    Kernel::UnitLabel unitLabel("Meters");
    MDHistoDimension dimension("Distance", "Dist", unitLabel, 0, 10, 1);
    const Mantid::Kernel::MDUnit &unit = dimension.getMDUnits();
    TS_ASSERT_EQUALS(unit.getUnitLabel(), unitLabel);
    TS_ASSERT(dynamic_cast<const Mantid::Kernel::LabelUnit *>(&unit));
  }

  void test_construct_with_frame_type() {
    QLab frame;
    MDHistoDimension dimension("QLabX", "QLabX", frame, 0, 10, 2);
    const auto &mdFrame = dimension.getMDFrame();

    TS_ASSERT_EQUALS(frame.name(), mdFrame.name());
    TS_ASSERT_EQUALS(frame.getUnitLabel(), mdFrame.getUnitLabel());

    std::string actualXML = dimension.toXMLString();

    std::string expectedXML =
        std::string("<Dimension ID=\"QLabX\">") + "<Name>QLabX</Name>" +
        "<Units>Angstrom^-1</Units>" + "<Frame>QLab</Frame>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>0.0000</LowerBounds>" + "<NumberOfBins>2</NumberOfBins>" +
        "</Dimension>";

    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }
};

#endif /* MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_ */
