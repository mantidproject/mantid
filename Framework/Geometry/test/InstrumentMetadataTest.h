// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument/InstrumentMetadata.h"
#include <cxxtest/TestSuite.h>

using Mantid::Geometry::InstrumentMetadata;
using Mantid::Types::Core::DateAndTime;

class InstrumentMetadataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentMetadataTest *createSuite() { return new InstrumentMetadataTest(); }
  static void destroySuite(InstrumentMetadataTest *suite) { delete suite; }

  void test_constructor_sets_values() {
    const auto metadata = createMetadata();
    TS_ASSERT_EQUALS(metadata.validFromDate(), DateAndTime("1900-01-31T23:59:59"));
    TS_ASSERT_EQUALS(metadata.validToDate(), DateAndTime("2100-01-31T23:59:59"));
    TS_ASSERT_EQUALS(metadata.filename(), "TEST_Definition.xml");
    TS_ASSERT_EQUALS(metadata.xmlText(), "<instrument/>");
    TS_ASSERT_EQUALS(metadata.defaultView(), "3D");
    TS_ASSERT_EQUALS(metadata.defaultAxis(), "Z+");
    TS_ASSERT(!metadata.physicalInstrument());
  }

  void test_setFileName() {
    auto metadata = createMetadata();
    metadata.setFileName("NEW_Definition.xml");
    TS_ASSERT_EQUALS(metadata.filename(), "NEW_Definition.xml");
  }

  void test_setXmlText() {
    auto metadata = createMetadata();
    metadata.setXmlText("<instrument name=\"NEW\"/>");
    TS_ASSERT_EQUALS(metadata.xmlText(), "<instrument name=\"NEW\"/>");
  }

  void test_setDefaultView() {
    auto metadata = createMetadata();
    metadata.setDefaultView("CYLINDRICAL_Y");
    TS_ASSERT_EQUALS(metadata.defaultView(), "CYLINDRICAL_Y");
  }

  void test_setDefaultViewAxis() {
    auto metadata = createMetadata();
    metadata.setDefaultViewAxis("X-");
    TS_ASSERT_EQUALS(metadata.defaultAxis(), "X-");
  }

  void test_setters_do_not_modify_other_values() {
    auto metadata = createMetadata();
    metadata.setFileName("NEW_Definition.xml");
    metadata.setXmlText("<instrument name=\"NEW\"/>");
    metadata.setDefaultView("CYLINDRICAL_Y");
    metadata.setDefaultViewAxis("X-");
    TS_ASSERT_EQUALS(metadata.validFromDate(), DateAndTime("1900-01-31T23:59:59"));
    TS_ASSERT_EQUALS(metadata.validToDate(), DateAndTime("2100-01-31T23:59:59"));
    TS_ASSERT(!metadata.physicalInstrument());
  }

private:
  InstrumentMetadata createMetadata() const {
    return InstrumentMetadata(DateAndTime("1900-01-31T23:59:59"), DateAndTime("2100-01-31T23:59:59"),
                              "TEST_Definition.xml", "<instrument/>", "3D", "Z+", nullptr);
  }
};
