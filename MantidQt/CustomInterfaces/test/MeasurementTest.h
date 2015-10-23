#ifndef MANTIDQT_CUSTOMINTERFACES_MEASUREMENTTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MEASUREMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Measurement.h"

using namespace MantidQt::CustomInterfaces;

class MeasurementTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeasurementTest *createSuite() { return new MeasurementTest(); }
  static void destroySuite(MeasurementTest *suite) { delete suite; }

  void test_invalid_construction_via_constructional_method() {
    std::string message = "Gave up";
    auto measure = Measurement::InvalidMeasurement(message);
    TS_ASSERT(!measure.isUseable());
    TS_ASSERT_EQUALS(message, measure.whyUnuseable());
  }

  void test_valid_construction_via_constructor() {
    const std::string measurementId = "a";
    const std::string measurementSubId = "s";
    const std::string measurementLabel = "l";
    const std::string measurementType = "t";
    const double angle = 0.1;
    const std::string run = "123";

    Measurement measurement(measurementId, measurementSubId, measurementLabel,
                            measurementType, angle, run);

    TS_ASSERT(measurement.isUseable());
    TS_ASSERT_EQUALS(measurementId, measurement.id());
    TS_ASSERT_EQUALS(measurementSubId, measurement.subId());
    TS_ASSERT_EQUALS(measurementLabel, measurement.label());
    TS_ASSERT_EQUALS(measurementType, measurement.type());
    TS_ASSERT_EQUALS(angle, measurement.angle());
    TS_ASSERT_EQUALS(run, measurement.run());
  }

  void test_invalid_construction_when_measurementId_empty() {

    Measurement measurement("", "measurementSubId", "measurementLabel",
                            "measurementType", 0.1, "111");

    TS_ASSERT(!measurement.isUseable());
  }

  void test_invalid_construction_when_measurementSubId_empty() {

    Measurement measurement("measurementId", "", "measurementLabel",
                            "measurementType", 0.1, "111");

    TS_ASSERT(!measurement.isUseable());
  }

  void test_valid_construction_when_label_empty() {

    Measurement measurement("measurementId", "measurementSubId", "",
                            "measurementType", 0.1, "111");

    TSM_ASSERT("Empty labels are not terminal", measurement.isUseable());
  }

  void test_valid_construction_when_type_empty() {
    Measurement measurement("measurementId", "measurementSubId",
                            "measurementLabel", "", 0.1, "111");

    TSM_ASSERT("Empty type info is not terminal",measurement.isUseable());
  }
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MEASUREMENTTEST_H_ */
