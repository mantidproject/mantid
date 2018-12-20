#ifndef MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTITEMSOURCETEST_H_
#define MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTITEMSOURCETEST_H_

#include "../ISISReflectometry/ReflNexusMeasurementItemSource.h"
#include "MantidAPI/FileFinder.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class ReflNexusMeasurementItemSourceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflNexusMeasurementItemSourceTest *createSuite() {
    return new ReflNexusMeasurementItemSourceTest();
  }
  static void destroySuite(ReflNexusMeasurementItemSourceTest *suite) {
    delete suite;
  }

  void test_obtain_via_full_path() {

    std::string path =
        Mantid::API::FileFinder::Instance().findRun("POLREF14966");
    Poco::File file(path);
    TSM_ASSERT("Test setup incorrect", !path.empty() && file.exists());

    ReflNexusMeasurementItemSource source;
    MeasurementItem measurementItem = source.obtain(path, "POLREF1111");
    TS_ASSERT(measurementItem.isUseable());
    TS_ASSERT(measurementItem.isUseable());
    TS_ASSERT_EQUALS("34", measurementItem.id());
    TS_ASSERT_EQUALS("0", measurementItem.subId());
    TS_ASSERT_EQUALS(
        "1111",
        measurementItem
            .run()); // Run number is taken from fuzzy because log is missing.
    TS_ASSERT_EQUALS("", measurementItem.label());
    TS_ASSERT_EQUALS("", measurementItem.label());
  }

  void test_obtain_via_fuzzy_path() {
    ReflNexusMeasurementItemSource source;

    MeasurementItem measurementItem = source.obtain("made_up", "POLREF14966");
    TS_ASSERT(measurementItem.isUseable());
    TS_ASSERT_EQUALS("34", measurementItem.id());
    TS_ASSERT_EQUALS("0", measurementItem.subId());
    TS_ASSERT_EQUALS("14966", measurementItem.run());
    TS_ASSERT_EQUALS("", measurementItem.label());
    TS_ASSERT_EQUALS("", measurementItem.label());
  }
};

#endif /* MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTITEMSOURCETEST_H_ */
