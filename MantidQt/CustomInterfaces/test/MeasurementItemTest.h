#ifndef MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEMTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Reflectometry/MeasurementItem.h"

using namespace MantidQt::CustomInterfaces;

class MeasurementItemTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeasurementItemTest *createSuite() {
    return new MeasurementItemTest();
  }
  static void destroySuite(MeasurementItemTest *suite) { delete suite; }

  void test_invalid_construction_via_constructional_method() {
    std::string message = "Gave up";
    auto measure = MeasurementItem::InvalidMeasurementItem(message);
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
    const std::string title = "title";

    MeasurementItem measurement(measurementId, measurementSubId,
                                measurementLabel, measurementType, angle, run,
                                title);

    TS_ASSERT(measurement.isUseable());
    TS_ASSERT_EQUALS(measurementId, measurement.id());
    TS_ASSERT_EQUALS(measurementSubId, measurement.subId());
    TS_ASSERT_EQUALS(measurementLabel, measurement.label());
    TS_ASSERT_EQUALS(measurementType, measurement.type());
    TS_ASSERT_EQUALS(angle, measurement.angle());
    TS_ASSERT_EQUALS(run, measurement.run());
  }

  void test_invalid_construction_when_measurementId_empty() {

    MeasurementItem measurement("", "measurementSubId", "measurementLabel",
                                "measurementType", 0.1, "111", "title");

    TS_ASSERT(!measurement.isUseable());
  }

  void test_invalid_construction_when_measurementSubId_empty() {

    MeasurementItem measurement("measurementId", "", "measurementLabel",
                                "measurementType", 0.1, "111", "title");

    TS_ASSERT(!measurement.isUseable());
  }

  void test_valid_construction_when_label_empty() {

    MeasurementItem measurement("measurementId", "measurementSubId", "",
                                "measurementType", 0.1, "111", "title");

    TSM_ASSERT("Empty labels are not terminal", measurement.isUseable());
  }

  void test_valid_construction_when_type_empty() {
    MeasurementItem measurement("measurementId", "measurementSubId",
                                "measurementLabel", "", 0.1, "111", "title");

    TSM_ASSERT("Empty type info is not terminal", measurement.isUseable());
  }

  void test_valid_construction_when_title_empty() {
    MeasurementItem measurement("measurementId", "measurementSubId",
                                "measurementLabel", "measurementType", 0.1,
                                "111", "");

    TSM_ASSERT("Empty run title is not terminal", measurement.isUseable());
  }
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEMTEST_H_ */
