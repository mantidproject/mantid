#ifndef MANTID_SUPPORTTEST_H_
#define MANTID_SUPPORTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/StringTokenizer.h"

/**
   \class StringTokenizerTest
   Checks the basic string operations in StringTokenizer
*/

class StringTokenizerTest : public CxxTest::TestSuite {
public:

    auto keyValues = splitToKeyValues("key1=value1, key2=value2");

    auto keyValues = splitToKeyValues("key1@value1, key2@value2", "@");

    auto keyValues = splitToKeyValues("key1@value1: key2@value2", "@", ":");


    auto keyValues = splitToKeyValues("key 1@value1: key2@value 2", "@", ":");



  void test_parseRange_defaultSimple() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result = parseRange("3,1,4,0,2,5"));
    std::vector<int> expected(6);
    expected[0] = 3;
    expected[1] = 1;
    expected[2] = 4;
    expected[3] = 0;
    expected[4] = 2;
    expected[5] = 5;
    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_defaultRanges() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(
        result = parseRange("  1, 2 - 5,   6   ,7,8,    9,10-12"));

    std::vector<int> expected;
    expected.reserve(12);
    for (int i = 1; i <= 12; i++)
      expected.push_back(i);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_emptyElements() {
    std::vector<int> expected(3);
    expected[0] = 1;
    expected[1] = 2;
    expected[2] = 3;

    std::vector<int> result1;
    TS_ASSERT_THROWS_NOTHING(result1 = parseRange(",1,2,3"));
    TS_ASSERT_EQUALS(result1, expected);

    std::vector<int> result2;
    TS_ASSERT_THROWS_NOTHING(result2 = parseRange("1,2,3,"));
    TS_ASSERT_EQUALS(result2, expected);

    std::vector<int> result3;
    TS_ASSERT_THROWS_NOTHING(result3 = parseRange("1,2,,,,3"));
    TS_ASSERT_EQUALS(result3, expected);
  }

  void test_parseRange_mapStyleSimple() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(
        result = parseRange("   52   53   54   55   56   57   58   192", " "));

    std::vector<int> expected;
    expected.reserve(8);
    for (int i = 52; i <= 58; i++)
      expected.push_back(i);
    expected.push_back(192);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_mapStyleRanges() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result =
                                 parseRange("  1- 3 4    5 - 7  8 -10  ", " "));

    std::vector<int> expected;
    expected.reserve(10);
    for (int i = 1; i <= 10; i++)
      expected.push_back(i);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_customRangeSep() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result =
                                 parseRange("1-2,3:5,6-7,8:10", ",", "-:"));

    std::vector<int> expected;
    expected.reserve(10);
    for (int i = 1; i <= 10; i++)
      expected.push_back(i);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_emptyString() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result = parseRange(""));
    TS_ASSERT(result.empty());
  }

  void test_parseRange_invalidElement() {
    TS_ASSERT_THROWS_EQUALS(parseRange("1,2,3,a,5"),
                            const std::invalid_argument &e, e.what(),
                            std::string("Invalid element: a"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1|,|3|4", "|"),
                            const std::invalid_argument &e, e.what(),
                            std::string("Invalid element: ,"));
  }

  void test_parseRange_invalidRange() {
    TS_ASSERT_THROWS_EQUALS(parseRange("1,2,3,-5,6"),
                            const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: -5"));

    TS_ASSERT_THROWS_EQUALS(parseRange("-3 4", " "),
                            const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: -3"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1,2,a-4,5"),
                            const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: a-4"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1,2-,5,6"),
                            const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: 2-"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1 5-", " "),
                            const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: 5-"));
  }

  void test_parseRange_multipleRangeSep() {
    TS_ASSERT_THROWS_EQUALS(parseRange("1--5 6  7", " "),
                            const std::invalid_argument &e, e.what(),
                            std::string("Multiple range separators: 1--5"));

    TS_ASSERT_THROWS_EQUALS(parseRange("----"), const std::invalid_argument &e,
                            e.what(),
                            std::string("Multiple range separators: ----"));
  }

  void test_parseRange_reversedRange() {
    TS_ASSERT_THROWS_EQUALS(parseRange("5-1,6,7"),
                            const std::invalid_argument &e, e.what(),
                            std::string("Range boundaries are reversed: 5-1"));
  }

};

#endif // MANTID_SUPPORTTEST_H_
