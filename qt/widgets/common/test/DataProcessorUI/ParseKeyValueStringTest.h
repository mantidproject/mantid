#ifndef MANTID_MANTIDWIDGETS_PARSEKEYVALUESTRINGTEST_H
#define MANTID_MANTIDWIDGETS_PARSEKEYVALUESTRINGTEST_H

#include "MantidQtWidgets/Common/DataProcessorUI/ParseKeyValueString.h"
#include <cxxtest/TestSuite.h>

class ParseKeyValueStringTest : public CxxTest::TestSuite {

public:
  void testParseKeyValueString() {
    std::map<std::string, std::string> kvp =
        MantidQt::MantidWidgets::parseKeyValueString(
            "a = 1,b=2.0, c=3, d='1,2,3',e=\"4,5,6\",f=1+1=2, g = '\\''");

    TS_ASSERT_EQUALS(kvp["a"], "1");
    TS_ASSERT_EQUALS(kvp["b"], "2.0");
    TS_ASSERT_EQUALS(kvp["c"], "3");
    TS_ASSERT_EQUALS(kvp["d"], "1,2,3");
    TS_ASSERT_EQUALS(kvp["e"], "4,5,6");
    TS_ASSERT_EQUALS(kvp["f"], "1+1=2");
    TS_ASSERT_EQUALS(kvp["g"], "'");

    TS_ASSERT_THROWS(
        MantidQt::MantidWidgets::parseKeyValueString("a = 1, b = 2, c = 3,"),
        std::runtime_error);
    TS_ASSERT_THROWS(
        MantidQt::MantidWidgets::parseKeyValueString("a = 1, b = 2, c = 3,d"),
        std::runtime_error);
    TS_ASSERT_THROWS(MantidQt::MantidWidgets::parseKeyValueString(",a = 1"),
                     std::runtime_error);
    TS_ASSERT_THROWS(
        MantidQt::MantidWidgets::parseKeyValueString(",a = 1 = 2,="),
        std::runtime_error);
    TS_ASSERT_THROWS(MantidQt::MantidWidgets::parseKeyValueString("=,=,="),
                     std::runtime_error);
  }
};

#endif // MANTID_MANTIDWIDGETS_PARSEKEYVALUESTRINGTEST_H
