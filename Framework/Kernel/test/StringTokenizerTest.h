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
  void test_tokenize_key_value() {

    auto tokenizer1 = Mantid::Kernel::StringTokenizer(
        "key1=value1: key2=value2", ":",
        Mantid::Kernel::StringTokenizer::TOK_TRIM);
    std::vector<std::string> result1(tokenizer1.begin(), tokenizer1.end());
    std::vector<std::string> expected1{"key1=value1", "key2=value2"};
    TS_ASSERT_EQUALS(result1, expected1);
    auto tokenizer2 = Mantid::Kernel::StringTokenizer(
        result1[0], "=", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    std::vector<std::string> result2(tokenizer2.begin(), tokenizer2.end());
    std::vector<std::string> expected2{"key1", "value1"};
    TS_ASSERT_EQUALS(result2, expected2);
    auto tokenizer3 = Mantid::Kernel::StringTokenizer(
        result1[1], "=", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    std::vector<std::string> result3(tokenizer3.begin(), tokenizer3.end());
    std::vector<std::string> expected3{"key2", "value2"};
    TS_ASSERT_EQUALS(result3, expected3);
  }

  void test_tokenize_key_value_with_spaces() {
    auto tokenizer1 =
        Mantid::Kernel::StringTokenizer("key 1@value1: key2@value 2", ":");
    std::vector<std::string> result1(tokenizer1.begin(), tokenizer1.end());
    std::vector<std::string> expected1{"key 1@value1", " key2@value 2"};
    TS_ASSERT_EQUALS(result1, expected1);

    auto tokenizer2 = Mantid::Kernel::StringTokenizer(result1[0], "@");
    std::vector<std::string> result2(tokenizer2.begin(), tokenizer2.end());
    std::vector<std::string> expected2{"key 1", "value1"};
    TS_ASSERT_EQUALS(result2, expected2);

    auto tokenizer3 = Mantid::Kernel::StringTokenizer(result1[1], "@");
    std::vector<std::string> result3(tokenizer3.begin(), tokenizer3.end());
    std::vector<std::string> expected3{" key2", "value 2"};
    TS_ASSERT_EQUALS(result3, expected3);
  }

  void test_parseRange_simple() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("3,1,4,0,2,5", ",");
    std::vector<std::string> result(tokenizer.begin(), tokenizer.end());
    std::vector<std::string> expected{"3", "1", "4", "0", "2", "5"};
    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_ranges() {
    auto tokenizer = Mantid::Kernel::StringTokenizer(
        "  1, 2 - 5,   6   ,7,8,    9,10-12", ",",
        Mantid::Kernel::StringTokenizer::TOK_TRIM |
            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    std::vector<std::string> result(tokenizer.begin(), tokenizer.end());
    std::vector<std::string> expected{"1", "2 - 5", "6",    "7",
                                      "8", "9",     "10-12"};
    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_emptyElements() {
    auto tokenizer1 = Mantid::Kernel::StringTokenizer(
        ",1,2,3", ",", Mantid::Kernel::StringTokenizer::TOK_TRIM |
                           Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    std::vector<std::string> result1(tokenizer1.begin(), tokenizer1.end());
    std::vector<std::string> expected1{"1", "2", "3"};
    TS_ASSERT_EQUALS(result1, expected1);

    auto tokenizer2 = Mantid::Kernel::StringTokenizer(
        "1,2,3,", ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    std::vector<std::string> result2(tokenizer2.begin(), tokenizer2.end());
    std::vector<std::string> expected2{"1", "2", "3", ""};
    TS_ASSERT_EQUALS(result2, expected2);

    auto tokenizer3 = Mantid::Kernel::StringTokenizer(
        "1,2,,,,3", ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    std::vector<std::string> result3(tokenizer3.begin(), tokenizer3.end());
    std::vector<std::string> expected3{"1", "2", "", "", "", "3"};
    TS_ASSERT_EQUALS(result3, expected3);
  }

  void test_parseRange_mapStyleSimple() {
    auto tokenizer1 = Mantid::Kernel::StringTokenizer(
        "   52   53   54   55   56   57   58   192", " ",
        Mantid::Kernel::StringTokenizer::TOK_TRIM |
            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    std::vector<std::string> result1(tokenizer1.begin(), tokenizer1.end());
    std::vector<std::string> expected1{"52", "53", "54", "55",
                                       "56", "57", "58", "192"};
    TS_ASSERT_EQUALS(result1, expected1);
  }

  void test_parseRange_customRangeSep() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("1-2,3:5,6-7,8:10", ",");
    std::vector<std::string> result(tokenizer.begin(), tokenizer.end());
    std::vector<std::string> expected{"1-2", "3:5", "6-7", "8:10"};
    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_emptyString() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("", "-:");
    TS_ASSERT_EQUALS(tokenizer.count(), 0);
  }

  void test_parseRange_invalidRange() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("1,2,3,-5,6", ",");
    std::vector<std::string> result(tokenizer.begin(), tokenizer.end());
    std::vector<std::string> expected{"1", "2", "3", "-5", "6"};
    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_invalidRange2() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("-5", "-");
    std::vector<std::string> result(tokenizer.begin(), tokenizer.end());
    std::vector<std::string> expected{"", "5"};
    TS_ASSERT_EQUALS(result, expected);
  }

  void test_parseRange_invalidRange3() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("2-", "-");
    std::vector<std::string> result(tokenizer.begin(), tokenizer.end());
    std::vector<std::string> expected{"2", ""};
    TS_ASSERT_EQUALS(result, expected);
  }

};

std::string random_string(size_t length) {
  auto randchar = []() -> char {
    const char charset[] = "0123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[rand() % max_index];
  };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}

class StringTokenizerTestPerformance : public CxxTest::TestSuite {
private:
public:
  void test_oneLargeString() {

    std::size_t length(200000000);
    std::string oneBigString = random_string(length);
    for (size_t i = 0; i < length; i += 10)
      oneBigString[i] = ';';
    // oneBigString.replace(i,1,1,';');
    auto start = std::chrono::system_clock::now();
    auto tokenizer1 = Mantid::Kernel::StringTokenizer(
        oneBigString, ";",
        Mantid::Kernel::StringTokenizer::TOK_TRIM |
            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    auto end = std::chrono::system_clock::now();
    TS_ASSERT_EQUALS(tokenizer1.count(), length / 10 + 1);
    std::chrono::duration<double> elapsed_seconds = end - start;

    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
    std::cout << "<" << tokenizer1[1] << " " << std::endl;
  }
};

#endif // MANTID_SUPPORTTEST_H_
