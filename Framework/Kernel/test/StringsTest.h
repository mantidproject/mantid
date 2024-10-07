// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidKernel/Strings.h"
#include <string>

using namespace Mantid::Kernel::Strings;

/**
   \class StringsTest
   \brief test of Strings components
   \date September 2005
   \author S.Ansell

   Checks the basic string operations in Strings
*/

class StringsTest : public CxxTest::TestSuite {
public:
  void test_replace() {
    std::string in = "hello\nI hate\nnewlines.\n";
    std::string out = replace(in, "\n", " ");
    TS_ASSERT_EQUALS(out, "hello I hate newlines. ");

    TS_ASSERT_EQUALS(replace("bla", "bla", ""), "");
    TS_ASSERT_EQUALS(replace("FirstSecond", "First", ""), "Second");
    TS_ASSERT_EQUALS(replace("FirstSecond", "Second", ""), "First");
    TS_ASSERT_EQUALS(replace("Hello You", " ", " I am stupid, "), "Hello I am stupid, You");
  }

  void test_replaceAll() {
    const std::string input = "Lots and lots of spaces in this sentence.";
    std::string out = replaceAll(input, " ", "_");
    std::string expected = "Lots_and_lots_of_spaces_in_this_sentence.";
    TS_ASSERT_EQUALS(out, expected);

    out = replaceAll(input, "L", "Lots and l");
    expected = "Lots and lots and lots of spaces in this sentence.";
    TS_ASSERT_EQUALS(out, expected);
  }

  void testSplitEmptyPath() {
    std::vector<std::string> result;
    TSM_ASSERT(" should return 0", !split_path("", result));
    TSM_ASSERT(" final path should be emtpy", result.empty());
  }
  void testSplitRemoveSpaces() {
    std::vector<std::string> result;
    TSM_ASSERT_EQUALS(" should return 1", 1, split_path("aaaa bbbb", result));
    TSM_ASSERT_EQUALS("should replace spaces", "aaaa_bbbb", result[0]);
  }
  void testSplitIn2() {
    std::vector<std::string> result;
    TSM_ASSERT_EQUALS("should split in 2", 2, split_path("aaaa\\bbbbb", result));
    TS_ASSERT_EQUALS("aaaa", result[0]);
    TS_ASSERT_EQUALS("bbbbb", result[1]);
  }
  void testSplitIn2IgnoreEdges() {
    std::vector<std::string> result;
    TSM_ASSERT_EQUALS("should split in 2", 2, split_path("/aaaa\\bbbbb/", result));
    TS_ASSERT_EQUALS("aaaa", result[0]);
    TS_ASSERT_EQUALS("bbbbb", result[1]);
  }
  void testSplitIn3IgnoreEdgesDelete1() {
    std::vector<std::string> result;
    TSM_ASSERT_EQUALS("should split in 2", 2, split_path("/aaaa\\bbbbb/./cccccc/../", result));
    TS_ASSERT_EQUALS("aaaa", result[0]);
    TS_ASSERT_EQUALS("bbbbb", result[1]);
  }
  void testSplitIn3IgnoreEdgesDelete1b() {
    std::vector<std::string> result;
    TSM_ASSERT_EQUALS("should split in 3", 3, split_path("/aaaa\\bbbbb/./cccccc/../ee", result));
    TS_ASSERT_EQUALS("aaaa", result[0]);
    TS_ASSERT_EQUALS("bbbbb", result[1]);
    TS_ASSERT_EQUALS("ee", result[2]);
  }
  void testSplitExpandFullPath() {
    Poco::Path test;
    test = test.absolute();
    std::string wkPath = test.toString();
    std::vector<std::string> base;
    size_t depth = split_path(wkPath, base);

    std::vector<std::string> result;
    TS_ASSERT_EQUALS(depth + 2, split_path("./aaaa\\bbbbb/./", result));
    TS_ASSERT_EQUALS("aaaa", result[depth]);
    TS_ASSERT_EQUALS("bbbbb", result[depth + 1]);
  }
  void testSplitExpandMoveUpPath() {
    Poco::Path test;
    test = test.absolute();
    std::string wkPath = test.toString();
    std::vector<std::string> base;
    size_t depth = split_path(wkPath, base);

    std::vector<std::string> result;
    TS_ASSERT_EQUALS(depth + 1, split_path("../aaaa\\bbbbb/./", result));
    TS_ASSERT_EQUALS("aaaa", result[depth - 1]);
    TS_ASSERT_EQUALS("bbbbb", result[depth]);
  }
  void testSplitTrhowOutOfrange() {
    std::vector<std::string> result;
    TSM_ASSERT_EQUALS("should return empty path", 0, split_path("/aaaa\\bbbbb/../../", result));
    TSM_ASSERT_EQUALS("should return empty path", 0, result.size());
    TSM_ASSERT_THROWS(" this path should go out of range", split_path("/aaaa\\bbbbb/../../../", result),
                      const std::invalid_argument &);
  }
  void testSkipLine() {
    TS_ASSERT(skipLine("#blah blah"));
    TS_ASSERT(!skipLine("blah blah"));
  }
  void testpeekLine() {
    std::istringstream text1("blah blah\nfoo bar\n");
    TS_ASSERT(peekLine(text1) == "blah blah");
    TS_ASSERT(peekLine(text1) == "blah blah");

    std::istringstream text2("\tblah blah \nfoo bar\n");
    TS_ASSERT(peekLine(text2) == "blah blah");
    TS_ASSERT(peekLine(text2) == "blah blah");
  }
  void testStrip() {
    TS_ASSERT(strip("blah") == "blah");
    TS_ASSERT(strip("   blah") == "blah");
    TS_ASSERT(strip("\nblah \t  ") == "blah");
    TS_ASSERT(strip("\tblah\t\n") == "blah");
  }

  void testExtractWord()
  /**
     Applies a test to the extractWord
     The object is to find a suitable lenght
     of a string in a group of words
     @retval -1 :: failed find word in string
     when the pattern exists.
  */
  {
    std::string Ln = "Name wav wavelength other stuff";
    int retVal = extractWord(Ln, "wavelengt", 4);
    TS_ASSERT_EQUALS(retVal, 1);
    TS_ASSERT_EQUALS(Ln, "Name wav  other stuff");
  }

  void testConvert() {

    int i;
    // valid double convert
    TS_ASSERT_EQUALS(convert("   568   ", i), 1);
    TS_ASSERT_EQUALS(i, 568);
    double X;
    // valid double convert
    TS_ASSERT_EQUALS(convert("   3.4   ", X), 1);
    TS_ASSERT_EQUALS(X, 3.4);
    X = 9.0;
    // invalid leading stuff
    TS_ASSERT_EQUALS(convert("   e3.4   ", X), 0);
    TS_ASSERT_EQUALS(X, 9.0);
    // invalid trailing stuff
    TS_ASSERT_EQUALS(convert("   3.4g   ", X), 0);
    TS_ASSERT_EQUALS(X, 9.0);
    std::string Y;
    TS_ASSERT_EQUALS(convert("   3.4y   ", Y), 1);
    TS_ASSERT_EQUALS(Y, "3.4y");
  }

  void testSection() {
    std::string Mline = "V 1 tth ";
    std::string Y;
    TS_ASSERT_EQUALS(section(Mline, Y), 1);
    TS_ASSERT_EQUALS(Y, "V");
    TS_ASSERT_EQUALS(Mline, " 1 tth "); // Note the non-remove spc
  }

  void testSectPartNum() {
    double X;
    std::string NTest = "   3.4   ";
    TS_ASSERT_EQUALS(sectPartNum(NTest, X), 1);
    TS_ASSERT_EQUALS(X, 3.4);
    X = 9.0;
    NTest = "   3.4g   ";
    TS_ASSERT_EQUALS(sectPartNum(NTest, X), 1);
    TS_ASSERT_EQUALS(X, 3.4);
    X = 9.0;
    NTest = "   e3.4   ";
    TS_ASSERT_DIFFERS(sectPartNum(NTest, X), 1);
    TS_ASSERT_EQUALS(X, 9.0);
  }

  void test_SplitToKeyValuePairs_Returns_Empty_Map_For_Empty_String() {
    auto keyValues = splitToKeyValues("");

    TS_ASSERT(keyValues.empty());
  }

  void test_SplitToKeyValuePairs_Returns_Empty_Map_For_String_With_No_Values() {
    auto keyValues = splitToKeyValues("key,key,key");

    TS_ASSERT(keyValues.empty());
  }

  void test_SplitToKeyValuePairs_Uses_Equals_And_Comma_As_Separators_By_Default() {
    auto keyValues = splitToKeyValues("key1=value1, key2=value2");

    TS_ASSERT_EQUALS(keyValues.size(), 2);
    TS_ASSERT_EQUALS(keyValues.at("key1"), "value1");
    TS_ASSERT_EQUALS(keyValues.at("key2"), "value2");
  }

  void test_SplitToKeyValuePairs_Uses_KeyValueSep_If_Given() {
    auto keyValues = splitToKeyValues("key1@value1, key2@value2", "@");

    TS_ASSERT_EQUALS(keyValues.size(), 2);
    TS_ASSERT_EQUALS(keyValues.at("key1"), "value1");
    TS_ASSERT_EQUALS(keyValues.at("key2"), "value2");
  }

  void test_SplitToKeyValuePairs_Uses_KeyValueSep_And_ListSep_If_Given() {
    auto keyValues = splitToKeyValues("key1@value1: key2@value2", "@", ":");

    TS_ASSERT_EQUALS(keyValues.size(), 2);
    TS_ASSERT_EQUALS(keyValues.at("key1"), "value1");
    TS_ASSERT_EQUALS(keyValues.at("key2"), "value2");
  }

  void test_SplitToKeyValuePairs_Does_Not_Ignore_Spaces_Within_Key_Or_Value() {
    auto keyValues = splitToKeyValues("key 1@value1: key2@value 2", "@", ":");

    TS_ASSERT_EQUALS(keyValues.size(), 2);
    TS_ASSERT_EQUALS(keyValues.at("key 1"), "value1");
    TS_ASSERT_EQUALS(keyValues.at("key2"), "value 2");
  }

  void test_SplitToKeyValuePairs_Ignores_Items_Without_A_Key_Or_Value() {
    auto keyValues = splitToKeyValues("key1=,key2=value2,=value3");

    TS_ASSERT_EQUALS(keyValues.size(), 1);
    TS_ASSERT_EQUALS(keyValues.at("key2"), "value2");
  }

  void test_join() {
    std::vector<std::string> v;
    std::string out;

    out = join(v.begin(), v.end(), ",");
    TS_ASSERT_EQUALS(out, "");

    v.emplace_back("Help");
    v.emplace_back("Me");
    v.emplace_back("I'm");
    v.emplace_back("Stuck");
    v.emplace_back("Inside");
    v.emplace_back("A");
    v.emplace_back("Test");

    out = join(v.begin(), v.end(), ",");
    TS_ASSERT_EQUALS(out, "Help,Me,I'm,Stuck,Inside,A,Test");
  }

  void test_joinSet() {
    std::set<std::string> v;
    std::string out;

    out = join(v.begin(), v.end(), ",");
    TS_ASSERT_EQUALS(out, "");

    v.insert("Help");
    v.insert("Me");
    v.insert("I'm");
    v.insert("Stuck");
    v.insert("Inside");
    v.insert("A");
    v.insert("Test");

    out = join(v.begin(), v.end(), ",");
    TS_ASSERT_EQUALS(out, "A,Help,I'm,Inside,Me,Stuck,Test");
  }

  void test_joinLong() {
    std::vector<std::string> v;
    std::string out;
    std::string ans;

    out = join(v.begin(), v.end(), ",");
    TS_ASSERT_EQUALS(out, "");

    int n = 100000;
    for (int i = 0; i < n; i++) {
      v.emplace_back(std::to_string(i));
      ans += std::to_string(i) + ",";
    }

    out = join(v.begin(), v.end(), ",");
    ans.pop_back();
    TS_ASSERT_EQUALS(out, ans);
  }

  void test_joinCompress() {

    std::vector<std::vector<int>> inputList{
        {1, 2, 3}, {-1, 0, 1}, {356, 366, 367, 368, 370, 371, 372, 375}, {7, 6, 5, 6, 7, 8, 10}};
    std::vector<std::string> resultList{"1-3", "-1-1", "356,366-368,370-372,375", "7,6,5-8,10"};

    for (size_t i = 0; i < inputList.size(); i++) {
      const auto &inputVector = inputList[i];
      TS_ASSERT_EQUALS(joinCompress(inputVector.begin(), inputVector.end(), ",", "-"), resultList[i]);
    }
  }

  void test_shorten() {

    std::vector<std::string> inputList{"",          // empty
                                       "1,2",       // shorter than the ellipsis
                                       "1,2,3",     // equal in length than the ellipsis
                                       "1,2,35",    // one longer than the ellipsis
                                       "1,2,3,4",   // two longer than the ellipsis
                                       "1,2,3,45",  // just long enough for the ellipsis
                                       "12,3,4,56", // another past the ellipsis
                                       "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20"};
    std::vector<std::string> resultListMaxLength7{"",        // empty
                                                  "1,2",     // shorter than the ellipsis
                                                  "1,2,3",   // equal in length than the ellipsis
                                                  "1,2,35",  // one longer than the ellipsis
                                                  "1,2,3,4", // two longer than the ellipsis
                                                  "1 ... 5", // just long enough for the ellipsis
                                                  "1 ... 6", // another past the ellipsis
                                                  "1 ... 0"};

    std::vector<std::string> resultListMaxLength20{"",          // empty
                                                   "1,2",       // shorter than the ellipsis
                                                   "1,2,3",     // equal in length than the ellipsis
                                                   "1,2,35",    // one longer than the ellipsis
                                                   "1,2,3,4",   // two longer than the ellipsis
                                                   "1,2,3,45",  // just long enough for the ellipsis
                                                   "12,3,4,56", // another past the ellipsis
                                                   "1,2,3,4 ... 8,19,20"};

    // test very short max size
    int maxLength = 7;
    for (size_t i = 0; i < inputList.size(); i++) {
      const auto &input = inputList[i];
      std::string result = shorten(input, maxLength);
      TS_ASSERT_EQUALS(result, resultListMaxLength7[i]);
      TS_ASSERT_LESS_THAN_EQUALS(result.size(), maxLength);
      TS_ASSERT_LESS_THAN_EQUALS(result.size(), input.size());
    }
    // test longer max size
    maxLength = 20;
    for (size_t i = 0; i < inputList.size(); i++) {
      const auto &input = inputList[i];
      std::string result = shorten(input, maxLength);
      TS_ASSERT_EQUALS(result, resultListMaxLength20[i]);
      TS_ASSERT_LESS_THAN_EQUALS(result.size(), maxLength);
      TS_ASSERT_LESS_THAN_EQUALS(result.size(), input.size());
    }
  }

  void test_endsWithInt() {
    TS_ASSERT_EQUALS(endsWithInt("pixel22"), 22);
    TS_ASSERT_EQUALS(endsWithInt("pixel000123"), 123);
    TS_ASSERT_EQUALS(endsWithInt("pixel99"), 99);
    TS_ASSERT_EQUALS(endsWithInt("bla123bla"), -1);
    TS_ASSERT_EQUALS(endsWithInt(""), -1);
    TS_ASSERT_EQUALS(endsWithInt("123bla"), -1);
    TS_ASSERT_EQUALS(endsWithInt("123b"), -1);
    TS_ASSERT_EQUALS(endsWithInt("123"), 123);
  }

  void test_isMember() {
    std::vector<std::string> group{"A", "A1", "B0", "C"};

    TS_ASSERT_EQUALS(1, isMember(group, "A1"));
    TS_ASSERT_EQUALS(-1, isMember(group, " "));
    TS_ASSERT_EQUALS(-1, isMember(group, "nothing"));
    TS_ASSERT_EQUALS(0, isMember(group, "A"));
    TS_ASSERT_EQUALS(3, isMember(group, "C"));
  }

  void test_parseRange_defaultSimple() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result = parseRange("3,1,4,0,2,5"));
    std::vector<int> expected{3, 1, 4, 0, 2, 5};
    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_defaultRanges() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result = parseRange("  1, 2 - 5,   6   ,7,8,    9,10-12"));

    std::vector<int> expected;
    expected.reserve(12);
    for (int i = 1; i <= 12; i++)
      expected.emplace_back(i);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_emptyElements() {
    std::vector<int> expected{1, 2, 3};

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
    TS_ASSERT_THROWS_NOTHING(result = parseRange("   52   53   54   55   56   57   58   192", " "));

    std::vector<int> expected;
    expected.reserve(8);
    for (int i = 52; i <= 58; i++)
      expected.emplace_back(i);
    expected.emplace_back(192);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_mapStyleRanges() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result = parseRange("  1- 3 4    5 - 7  8 -10  ", " "));

    std::vector<int> expected;
    expected.reserve(10);
    for (int i = 1; i <= 10; i++)
      expected.emplace_back(i);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_customRangeSep() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result = parseRange("1-2,3:5,6-7,8:10", ",", "-:"));

    std::vector<int> expected;
    expected.reserve(10);
    for (int i = 1; i <= 10; i++)
      expected.emplace_back(i);

    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_emptyString() {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(result = parseRange(""));
    TS_ASSERT(result.empty());
  }

  void test_parseRange_invalidElement() {
    TS_ASSERT_THROWS_EQUALS(parseRange("1,2,3,a,5"), const std::invalid_argument &e, e.what(),
                            std::string("Invalid element: a"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1|,|3|4", "|"), const std::invalid_argument &e, e.what(),
                            std::string("Invalid element: ,"));
  }

  void test_parseRange_invalidRange() {
    TS_ASSERT_THROWS_EQUALS(parseRange("1,2,3,-5,6"), const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: -5"));

    TS_ASSERT_THROWS_EQUALS(parseRange("-3 4", " "), const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: -3"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1,2,a-4,5"), const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: a-4"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1,2-,5,6"), const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: 2-"));

    TS_ASSERT_THROWS_EQUALS(parseRange("1 5-", " "), const std::invalid_argument &e, e.what(),
                            std::string("Invalid range: 5-"));
  }

  void test_parseRange_multipleRangeSep() {
    TS_ASSERT_THROWS_EQUALS(parseRange("1--5 6  7", " "), const std::invalid_argument &e, e.what(),
                            std::string("Multiple range separators: 1--5"));

    TS_ASSERT_THROWS_EQUALS(parseRange("----"), const std::invalid_argument &e, e.what(),
                            std::string("Multiple range separators: ----"));
  }

  void test_parseRange_reversedRange() {
    TS_ASSERT_THROWS_EQUALS(parseRange("5-1,6,7"), const std::invalid_argument &e, e.what(),
                            std::string("Range boundaries are reversed: 5-1"));
  }

  void test_parseGroups_emptyString() {
    std::vector<std::vector<int>> result;
    TS_ASSERT_THROWS_NOTHING(result = parseGroups<int>(""))
    TS_ASSERT(result.empty());
  }

  void test_parseGroups_comma() {
    std::vector<std::vector<int>> result;
    TS_ASSERT_THROWS_NOTHING(result = parseGroups<int>("7,13"))
    std::vector<std::vector<int>> expected{{std::vector<int>(1, 7), std::vector<int>(1, 13)}};
    TS_ASSERT_EQUALS(result, expected)
  }

  void test_parseGroups_plus() {
    std::vector<std::vector<int>> result;
    TS_ASSERT_THROWS_NOTHING(result = parseGroups<int>("7+13"))
    std::vector<std::vector<int>> expected{{std::vector<int>()}};
    expected.front().emplace_back(7);
    expected.front().emplace_back(13);
    TS_ASSERT_EQUALS(result, expected)
  }

  void test_parseGroups_dash() {
    std::vector<std::vector<int>> result;
    TS_ASSERT_THROWS_NOTHING(result = parseGroups<int>("7-13"))
    std::vector<std::vector<int>> expected{{std::vector<int>()}};
    for (int i = 7; i <= 13; ++i) {
      expected.front().emplace_back(i);
    }
    TS_ASSERT_EQUALS(result, expected)
  }

  void test_parseGroups_complexExpression() {
    std::vector<std::vector<int>> result;
    TS_ASSERT_THROWS_NOTHING(result = parseGroups<int>("1,4+5+8,7-13,1"))
    std::vector<std::vector<int>> expected;
    expected.emplace_back(1, 1);
    expected.emplace_back();
    expected.back().emplace_back(4);
    expected.back().emplace_back(5);
    expected.back().emplace_back(8);
    expected.emplace_back();
    for (int i = 7; i <= 13; ++i) {
      expected.back().emplace_back(i);
    }
    expected.emplace_back(1, 1);
    TS_ASSERT_EQUALS(result, expected)
  }

  void test_parseGroups_acceptsWhitespace() {
    std::vector<std::vector<int>> result;
    TS_ASSERT_THROWS_NOTHING(result = parseGroups<int>(" 1\t, 4 +  5\t+ 8 , 7\t- 13 ,\t1  "))
    std::vector<std::vector<int>> expected;
    expected.emplace_back(1, 1);
    expected.emplace_back();
    expected.back().emplace_back(4);
    expected.back().emplace_back(5);
    expected.back().emplace_back(8);
    expected.emplace_back();
    for (int i = 7; i <= 13; ++i) {
      expected.back().emplace_back(i);
    }
    expected.emplace_back(1, 1);
    TS_ASSERT_EQUALS(result, expected)
  }

  void test_parseGroups_throwsWhenInputContainsNonnumericCharacters() {
    TS_ASSERT_THROWS_EQUALS(parseGroups<int>("a"), const std::runtime_error &e, e.what(),
                            std::string("Cannot parse numbers from string: 'a'"))
  }

  void test_parseGroups_throwsWhenOperationsAreInvalid() {
    TS_ASSERT_THROWS_EQUALS(parseGroups<int>("-1"), const std::runtime_error &e, e.what(),
                            std::string("Malformed range (-) operation."))
    TS_ASSERT_THROWS_EQUALS(parseGroups<int>(":1"), const std::runtime_error &e, e.what(),
                            std::string("Malformed range (:) operation."))
  }

  void test_toString_vector_of_ints() {
    std::vector<int> sortedInts{1, 2, 3, 5, 6, 8};
    auto result = toString(sortedInts);
    TS_ASSERT_EQUALS(std::string("1-3,5-6,8"), result);
  }

  void test_getLine() {
    std::istringstream text("blah blah\nfoo bar#comment\n");
    std::string line = getLine(text);
    TSM_ASSERT_EQUALS("Strings::getLine failed to read the first line.", line, "blah blah");
    getLine(text, line);
    TSM_ASSERT_EQUALS("Strings::getLine failed to remove comment.", line, "foo bar");
    getLine(text, line);
    TSM_ASSERT_EQUALS("Strings::getLine didn't return empty string after eof.", line, "");
  }

  void test_randomString() {
    for (size_t length = 0; length < 5; ++length) {
      const auto text = randomString(length);
      TS_ASSERT_EQUALS(text.length(), length);
    }
  }

  void test_endsWith_when_the_suffix_is_smaller_than_the_str() {
    TS_ASSERT(endsWith("ATestString", "String"));
    TS_ASSERT(!endsWith("AStringTest", "String"));
  }

  void test_endsWith_when_the_suffix_is_too_large() { TS_ASSERT(!endsWith("SmallText", "AVeryLongSuffix")); }
};

class StringsTestPerformance : public CxxTest::TestSuite {
public:
  static StringsTestPerformance *createSuite() { return new StringsTestPerformance(); }
  static void destroySuite(StringsTestPerformance *suite) { delete suite; }
  void setUp() override { input = std::vector<double>(50000000, 0.123456); }
  void test_join_double() { auto result = join(input.begin(), input.end(), separator); }

private:
  std::vector<double> input;
  std::string separator{","};
};
