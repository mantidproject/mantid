// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDERTEST_H_
#define MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDERTEST_H_

#include "MantidGeometry/Instrument.h"
#include "MantidNexusGeometry/JSONGeometryParser.h"
#include "MantidNexusGeometry/JSONInstrumentBuilder.h"
#include "MantidTestHelpers/JSONGeometryParserTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <json/json.h>

using namespace Mantid;
using Mantid::NexusGeometry::JSONInstrumentBuilder;

class JSONInstrumentBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static JSONInstrumentBuilderTest *createSuite() {
    return new JSONInstrumentBuilderTest();
  }
  static void destroySuite(JSONInstrumentBuilderTest *suite) { delete suite; }

  void test_constructor_pass_valid_instrument() {
    auto json = TestHelpers::getFullJSONInstrumentSimpleWithChopper();
    TS_ASSERT_THROWS_NOTHING((JSONInstrumentBuilder(json)));
  }

  void test_constructor_fail_invalid_instrument() {
    TS_ASSERT_THROWS(JSONInstrumentBuilder(""), const std::invalid_argument &);
  }

  void test_build_geometry() {
    auto json = TestHelpers::getFullJSONInstrumentSimpleWithChopper();
    JSONInstrumentBuilder builder(json);
    TS_ASSERT_THROWS_NOTHING((builder.buildGeometry()));
  }

  void test_simple_instrument() {
    auto json = TestHelpers::getFullJSONInstrumentSimpleWithMonitor();
    JSONInstrumentBuilder builder(json);
    auto instrument = builder.buildGeometry();

    TS_ASSERT_EQUALS(instrument->getFullName(), "SimpleInstrument");
    TS_ASSERT(instrument->getComponentByName("detector_1"));
    TS_ASSERT_EQUALS(instrument->getNumberDetectors(false), 5); // 5 with
                                                                // monitor

    auto sample = instrument->getSample();
    TS_ASSERT_EQUALS(sample->getName(), "sample");

    detid_t min_id;
    detid_t max_id;
    instrument->getMinMaxDetectorIDs(min_id, max_id);
    TS_ASSERT_EQUALS(min_id, 1);
    TS_ASSERT_EQUALS(max_id, 90000); // monitor
  }
};

#endif /* MANTID_NEXUSGEOMETRY_JSONINSTRUMENTBUILDERTEST_H_ */
