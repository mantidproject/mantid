// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid;
using namespace Mantid::Geometry;

class MDHistoDimensionTest : public CxxTest::TestSuite {
public:
  void test_constructor_throws() {
    coord_t min = 10;
    coord_t max = 1; // min > max !
    Mantid::Geometry::GeneralFrame frame("My General Frame", "Furlongs");
    TSM_ASSERT_THROWS("Should throw if min > max!", MDHistoDimension("name", "id", frame, min, max, 15),
                      const std::invalid_argument &);
  }

  void test_constructor() {
    Mantid::Geometry::GeneralFrame frame("My General Frame", "Furlongs");
    MDHistoDimension d("name", "id", frame, -10, 20.0, 15);
    TS_ASSERT_EQUALS(d.getName(), "name");
    TS_ASSERT_EQUALS(d.getDimensionId(), "id");
    TS_ASSERT_EQUALS(d.getUnits(), "Furlongs");
    TS_ASSERT_EQUALS(d.getMinimum(), -10);
    TS_ASSERT_EQUALS(d.getMaximum(), +20);
    TS_ASSERT_EQUALS(d.getNBins(), 15);
    TS_ASSERT_EQUALS(d.getNBoundaries(), 16);
    TS_ASSERT_DELTA(d.getBinWidth(), 2.0, 1e-5);
  }

  void test_toXMLStringIntegrated() {
    std::string expectedXML = std::string("<Dimension ID=\"id\">") + "<Name>name</Name>" + "<Units>Furlongs</Units>" +
                              "<Frame>My General Frame</Frame>" + "<UpperBounds>20.0000</UpperBounds>" +
                              "<LowerBounds>-10.0000</LowerBounds>" + "<NumberOfBins>1</NumberOfBins>" +
                              "<Integrated>" + "<UpperLimit>20.0000</UpperLimit>" +
                              "<LowerLimit>-10.0000</LowerLimit>" + "</Integrated>" + "</Dimension>";
    Mantid::Geometry::GeneralFrame frame("My General Frame", "Furlongs");
    MDHistoDimension dimension("name", "id", frame, -10, 20.0, 1);
    std::string actualXML = dimension.toXMLString();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }

  void test_toXMLStringNotIntegrated() {
    std::string expectedXML = std::string("<Dimension ID=\"id\">") + "<Name>name</Name>" + "<Units>Furlongs</Units>" +
                              "<Frame>My General Frame</Frame>" + "<UpperBounds>20.0000</UpperBounds>" +
                              "<LowerBounds>-10.0000</LowerBounds>" + "<NumberOfBins>15</NumberOfBins>" +
                              "</Dimension>";
    Mantid::Geometry::GeneralFrame frame("My General Frame", "Furlongs");
    MDHistoDimension dimension("name", "id", frame, -10, 20.0, 15);
    std::string actualXML = dimension.toXMLString();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }

  void test_getMDUnits_gives_label_unit() {
    Kernel::UnitLabel unitLabel("Meters");
    Mantid::Geometry::GeneralFrame frame("Length", unitLabel);
    MDHistoDimension dimension("Distance", "Dist", frame, 0, 10, 1);
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

    std::string expectedXML = std::string("<Dimension ID=\"QLabX\">") + "<Name>QLabX</Name>" +
                              "<Units>Angstrom^-1</Units>" + "<Frame>QLab</Frame>" +
                              "<UpperBounds>10.0000</UpperBounds>" + "<LowerBounds>0.0000</LowerBounds>" +
                              "<NumberOfBins>2</NumberOfBins>" + "</Dimension>";

    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }

  void test_reset_the_md_frame_type() {
    // Arrange
    Kernel::UnitLabel unitLabel("Meters");
    Mantid::Geometry::GeneralFrame frameGeneral("Length", unitLabel);
    Mantid::Geometry::QLab frameQLab;
    MDHistoDimension dimension("Distance", "Dist", frameGeneral, 0, 10, 1);
    // Act
    dimension.setMDFrame(frameQLab);
    // Assert
    TSM_ASSERT_EQUALS("Should now be a QLab frame", dimension.getMDFrame().name(), Mantid::Geometry::QLab::QLabName);
  }

  void test_setName() {
    Mantid::Geometry::GeneralFrame frame("My General Frame", "Furlongs");
    MDHistoDimension dimension("name", "id", frame, -10, 20.0, 15);
    dimension.setName("renamed");
    TS_ASSERT_EQUALS(dimension.getName(), "renamed");
    // The id is independent of the name and must be untouched.
    TS_ASSERT_EQUALS(dimension.getDimensionId(), "id");
  }

  void test_setUnits_general_frame() {
    Mantid::Geometry::GeneralFrame frame("Length", "Meters");
    MDHistoDimension dimension("Distance", "Dist", frame, 0, 10, 1);
    dimension.setUnits(Kernel::UnitLabel("Furlongs"));
    TS_ASSERT_EQUALS(dimension.getUnits(), "Furlongs");
  }

  void test_setUnits_hkl_conforming_label() {
    HKL frame(new Mantid::Kernel::ReciprocalLatticeUnit);
    MDHistoDimension dimension("H", "H", frame, 0, 1, 5);
    dimension.setUnits(Kernel::UnitLabel("in 2.5 A^-1"));
    TS_ASSERT_EQUALS(dimension.getUnits(), "in 2.5 A^-1");
    // The unit must remain a Q unit so the special coordinate system is still detected.
    TSM_ASSERT("HKL dimension should still report a Q unit", dimension.getMDUnits().isQUnit());
  }

  void test_setUnits_hkl_nonconforming_label_throws_and_leaves_dimension_unchanged() {
    HKL frame(new Mantid::Kernel::ReciprocalLatticeUnit);
    MDHistoDimension dimension("H", "H", frame, 0, 1, 5);
    const auto originalUnits = dimension.getUnits();
    TS_ASSERT_THROWS(dimension.setUnits(Kernel::UnitLabel("myaxis")), const std::invalid_argument &);
    TSM_ASSERT_EQUALS("A rejected label must leave the units unchanged", dimension.getUnits(), originalUnits);
  }

  void test_setUnits_hkl_canonical_rlu_label() {
    HKL frame(new Mantid::Kernel::ReciprocalLatticeUnit);
    MDHistoDimension dimension("H", "H", frame, 0, 1, 5);
    dimension.setUnits(Kernel::UnitLabel("r.l.u."));
    TS_ASSERT_EQUALS(dimension.getUnits(), "r.l.u.");
    TSM_ASSERT("HKL dimension should still report a Q unit", dimension.getMDUnits().isQUnit());
  }

  void test_setUnits_qsample_throws() {
    Mantid::Geometry::QSample frame;
    MDHistoDimension dimension("Q", "Q", frame, 0, 1, 5);
    TS_ASSERT_THROWS(dimension.setUnits(Kernel::UnitLabel("in 2.5 A^-1")), const std::invalid_argument &);
  }

  void test_setUnits_qlab_throws() {
    Mantid::Geometry::QLab frame;
    MDHistoDimension dimension("Q", "Q", frame, 0, 1, 5);
    TS_ASSERT_THROWS(dimension.setUnits(Kernel::UnitLabel("in 2.5 A^-1")), const std::invalid_argument &);
  }
};
