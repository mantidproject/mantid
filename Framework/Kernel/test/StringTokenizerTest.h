#ifndef MANTID_STRINGTOKENIZERTEST_H_
#define MANTID_STRINGTOKENIZERTEST_H_

#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/uniform_int_distribution.h"
#include <array>
#include <cxxtest/TestSuite.h>
#include <random>

/**
   \class StringTokenizerTest
   Checks the basic string operations in StringTokenizer
*/

class StringTokenizerTest : public CxxTest::TestSuite {
public:
  void test_StringTokenizer_key_value() {
    auto tokenizer1 = Mantid::Kernel::StringTokenizer(
        "key1=value1: key2=value2", ":",
        Mantid::Kernel::StringTokenizer::TOK_TRIM);
    std::vector<std::string> expected{"key1=value1", "key2=value2"};
    TS_ASSERT_EQUALS(tokenizer1.asVector(), expected);
    auto tokenizer2 = Mantid::Kernel::StringTokenizer(
        tokenizer1[0], "=", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    expected = {"key1", "value1"};
    TS_ASSERT_EQUALS(tokenizer1.count(), 2);
    TS_ASSERT_EQUALS(tokenizer2.asVector(), expected);
    tokenizer2 = Mantid::Kernel::StringTokenizer(
        tokenizer1[1], "=", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    expected = {"key2", "value2"};
    TS_ASSERT_EQUALS(tokenizer2.asVector(), expected);
  }

  void test_StringTokenizer_key_value_with_spaces() {
    auto tokenizer1 =
        Mantid::Kernel::StringTokenizer("key 1@value1: key2@value 2", ":");
    std::vector<std::string> expected1{"key 1@value1", " key2@value 2"};
    TS_ASSERT_EQUALS(tokenizer1.asVector(), expected1);
    TS_ASSERT_EQUALS(tokenizer1.count(), 2);

    auto tokenizer2 = Mantid::Kernel::StringTokenizer(tokenizer1[0], "@");
    std::vector<std::string> expected2{"key 1", "value1"};
    TS_ASSERT_EQUALS(tokenizer2.asVector(), expected2);

    tokenizer2 = Mantid::Kernel::StringTokenizer(tokenizer1.at(0), "@");
    expected2 = {"key 1", "value1"};
    TS_ASSERT_EQUALS(tokenizer2.asVector(), expected2);

    tokenizer2 = Mantid::Kernel::StringTokenizer(tokenizer1[1], "@");
    expected2 = {" key2", "value 2"};
    TS_ASSERT_EQUALS(tokenizer2.asVector(), expected2);

    tokenizer2 = Mantid::Kernel::StringTokenizer(tokenizer1.at(1), "@");
    expected2 = {" key2", "value 2"};
    TS_ASSERT_EQUALS(tokenizer2.asVector(), expected2);

    TS_ASSERT_THROWS_ANYTHING(tokenizer1.at(3));
  }

  void test_StringTokenizer_parseRange_simple() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("3,1,4,0,2,5", ",");
    std::vector<std::string> expected{"3", "1", "4", "0", "2", "5"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
    tokenizer = Mantid::Kernel::StringTokenizer("3,1,4,0,2,5,", ",");
    expected = {"3", "1", "4", "0", "2", "5", ""};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
    tokenizer = Mantid::Kernel::StringTokenizer(
        "3,1,4,0,2,5,", ",",
        Mantid::Kernel::StringTokenizer::TOK_IGNORE_FINAL_EMPTY_TOKEN);
    expected = {"3", "1", "4", "0", "2", "5"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_parseRange_ranges() {
    auto tokenizer = Mantid::Kernel::StringTokenizer(
        "  1, 2 - 5,   6   ,7,8,    9,10-12", ",",
        Mantid::Kernel::StringTokenizer::TOK_TRIM |
            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    std::vector<std::string> expected{"1", "2 - 5", "6",    "7",
                                      "8", "9",     "10-12"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_parseRange_emptyElements() {
    auto tokenizer = Mantid::Kernel::StringTokenizer(
        ",1,2,3", ",",
        Mantid::Kernel::StringTokenizer::TOK_TRIM |
            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    std::vector<std::string> expected{"1", "2", "3"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);

    tokenizer = Mantid::Kernel::StringTokenizer(
        "1,2,3,", ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    expected = {"1", "2", "3", ""};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);

    tokenizer = Mantid::Kernel::StringTokenizer(
        "1,2,,,,3", ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    expected = {"1", "2", "", "", "", "3"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_parseRange_mapStyleSimple() {
    auto tokenizer = Mantid::Kernel::StringTokenizer(
        "   52   53   54   55   56   57   58   192", " ",
        Mantid::Kernel::StringTokenizer::TOK_TRIM |
            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    std::vector<std::string> expected{"52", "53", "54", "55",
                                      "56", "57", "58", "192"};
    TS_ASSERT_EQUALS(tokenizer.count(), 8);
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_parseRange_customRangeSep() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("1-2,3:5,6-7,8:10", ",");
    std::vector<std::string> expected{"1-2", "3:5", "6-7", "8:10"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_parseRange_emptyString() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("", "-:");
    TS_ASSERT_EQUALS(tokenizer.count(), 0);
    TS_ASSERT_EQUALS(tokenizer.asVector(), std::vector<std::string>());
  }

  void test_StringTokenizer_parseRange_invalidRange() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("1,2,3,-5,6", ",");
    std::vector<std::string> expected{"1", "2", "3", "-5", "6"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_parseRange_invalidRange2() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("-5", "-");
    std::vector<std::string> expected{"", "5"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_parseRange_invalidRange3() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("2-", "-");
    std::vector<std::string> expected{"2", ""};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
    tokenizer = Mantid::Kernel::StringTokenizer(
        "2-", "-",
        Mantid::Kernel::StringTokenizer::TOK_IGNORE_FINAL_EMPTY_TOKEN);
    expected = {"2"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_invalidOptionThrows() {
    TS_ASSERT_THROWS_ANYTHING(Mantid::Kernel::StringTokenizer(" ", "-:", 8));
  }

  void test_StringTokenizer_multipleSeparators() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("1,2,3,-5,6", ",-");
    std::vector<std::string> expected{"1", "2", "3", "", "5", "6"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }

  void test_StringTokenizer_emptySeparators() {
    auto tokenizer = Mantid::Kernel::StringTokenizer("1,2,3,-5,6", "");
    std::vector<std::string> expected{"1,2,3,-5,6"};
    TS_ASSERT_EQUALS(tokenizer.asVector(), expected);
  }
};

class RandomCharacter {
public:
  RandomCharacter() {
    const size_t maxIndex = (sizeof(m_characterSet) - 1);
    m_distribution =
        Mantid::Kernel::uniform_int_distribution<unsigned>(0, maxIndex);
  }
  char operator()() { return m_characterSet[m_distribution(m_generator)]; }

private:
  std::array<char, 62> m_characterSet{
      {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
       'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
       'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
       'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
       'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'}};
  std::mt19937 m_generator;
  Mantid::Kernel::uniform_int_distribution<unsigned> m_distribution;
};

std::string randomString(size_t length) {
  RandomCharacter randChar;
  std::string str(length, 0);
  for (auto &character : str) {
    character = randChar();
  }
  return str;
}

class StringTokenizerTestPerformance : public CxxTest::TestSuite {
private:
  std::string m_bigString;
  std::size_t m_length = 50000000;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StringTokenizerTestPerformance *createSuite() {
    return new StringTokenizerTestPerformance();
  }
  static void destroySuite(StringTokenizerTestPerformance *suite) {
    delete suite;
  }

  StringTokenizerTestPerformance() {
    m_bigString = randomString(m_length);
    for (size_t i = 2; i < m_length; i += 10) {
      m_bigString[i - 2] = ';';
      m_bigString[i - 1] = ' ';
      m_bigString[i] = ';';
    }
  };

  void test_oneLargeString() {
    auto tokenizer1 = Mantid::Kernel::StringTokenizer(m_bigString, ";", 0);
    TS_ASSERT_EQUALS(tokenizer1.count(), m_length / 5 + 1);
  }

  void test_oneLargeString_trim() {
    auto tokenizer1 = Mantid::Kernel::StringTokenizer(
        m_bigString, ";", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    TS_ASSERT_EQUALS(tokenizer1.count(), m_length / 5 + 1);
  }

  void test_oneLargeString_trim_ignoreEmpty() {
    auto tokenizer1 = Mantid::Kernel::StringTokenizer(
        m_bigString, ";",
        Mantid::Kernel::StringTokenizer::TOK_TRIM |
            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    TS_ASSERT_EQUALS(tokenizer1.count(), m_length / 10);
  }
};

#endif // MANTID_STRINGTOKENZIERTEST_H_
