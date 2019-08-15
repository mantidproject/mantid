// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_PARSEKEYVALUESTRINGTEST_H
#define MANTID_MANTIDWIDGETS_PARSEKEYVALUESTRINGTEST_H

#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <cxxtest/TestSuite.h>
#include <functional>

using MantidQt::MantidWidgets::parseKeyValueQString;
using MantidQt::MantidWidgets::parseKeyValueString;

class ParseKeyValueStringTest : public CxxTest::TestSuite {
public:
  using StdMap = std::map<std::string, std::string>;
  using QtMap = std::map<QString, QString>;
  static constexpr char *TEST_STRING =
      R"(a = 1,b=2.0, c=3, d='1,2,3',e="4,5,6",f=1+1=2, g = '\'')";

  void testStdMapParsedOutputsAreCorrect() {
    StdMap kvp = parseKeyValueString(TEST_STRING);
    assertTestStringOutputsAreCorrect(kvp);
  }

  void testQtMapParsedOutputsAreCorrect() {
    QtMap kvp = parseKeyValueQString(TEST_STRING);
    assertTestStringOutputsAreCorrect(kvp);
  }

  void testStdMapParseKeyValueStringThrowsIfTrailingSeparator() {
    TS_ASSERT_THROWS(parseKeyValueString("a = 1, b = 2, c = 3,"),
                     const std::runtime_error &);
  }

  void testQtMapParseKeyValueStringThrowsIfTrailingSeparator() {
    TS_ASSERT_THROWS(parseKeyValueQString("a = 1, b = 2, c = 3,"),
                     const std::runtime_error &);
  }

  void testStdParseKeyValueStringThrowsIfNotKeyValuePair() {
    TS_ASSERT_THROWS(parseKeyValueString("a = 1, b = 2, c = 3,d"),
                     const std::runtime_error &);
  }

  void testQtParseKeyValueStringThrowsIfNotKeyValuePair() {
    TS_ASSERT_THROWS(parseKeyValueQString("a = 1, b = 2, c = 3,d"),
                     const std::runtime_error &);
  }

  void testStdParseKeyValueStringThrowsIfStartsWithSeparator() {
    TS_ASSERT_THROWS(parseKeyValueString(",a = 1"), const std::runtime_error &);
  }

  void testQtParseKeyValueStringThrowsIfStartsWithSeparator() {
    TS_ASSERT_THROWS(parseKeyValueQString(",a = 1"),
                     const std::runtime_error &);
  }

  void
  testStdParseKeyValueStringThrowsIfStartsWithSeparatorAndEndsWithEquals() {
    TS_ASSERT_THROWS(parseKeyValueString(",a = 1 = 2,="),
                     const std::runtime_error &);
  }

  void testQtParseKeyValueStringThrowsIfStartsWithSeparatorAndEndsWithEquals() {
    TS_ASSERT_THROWS(parseKeyValueQString(",a = 1 = 2,="),
                     const std::runtime_error &);
  }

  void testStdParseKeyValueStringThrowsIfValuesOnlyContainEquals() {
    TS_ASSERT_THROWS(parseKeyValueString("=,=,="), const std::runtime_error &);
  }

  void testQtParseKeyValueStringThrowsIfValuesOnlyContainEquals() {
    TS_ASSERT_THROWS(parseKeyValueQString("=,=,="), const std::runtime_error &);
  }

  void testStdParseKeyValueStringWithCustomSeparator() {
    StdMap kvp = parseKeyValueString(R"(a=1;b=2)", ";");
    TS_ASSERT_EQUALS(kvp["a"], "1");
    TS_ASSERT_EQUALS(kvp["b"], "2");
  }

  void testQtParseKeyValueStringWithCustomSeparator() {
    QtMap kvp = parseKeyValueQString(R"(a=1;b=2)", ";");
    TS_ASSERT_EQUALS(kvp["a"], "1");
    TS_ASSERT_EQUALS(kvp["b"], "2");
  }

private:
  template <class StringType>
  void assertTestStringOutputsAreCorrect(std::map<StringType, StringType> kvp) {
    TS_ASSERT_EQUALS(kvp["a"], "1");
    TS_ASSERT_EQUALS(kvp["b"], "2.0");
    TS_ASSERT_EQUALS(kvp["c"], "3");
    TS_ASSERT_EQUALS(kvp["d"], "1,2,3");
    TS_ASSERT_EQUALS(kvp["e"], "4,5,6");
    TS_ASSERT_EQUALS(kvp["f"], "1+1=2");
    TS_ASSERT_EQUALS(kvp["g"], "'");
  }
};

#endif // MANTID_MANTIDWIDGETS_PARSEKEYVALUESTRINGTEST_H
