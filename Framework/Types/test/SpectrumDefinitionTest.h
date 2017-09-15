#ifndef MANTID_TYPES_SPECTRUMDEFINITIONTEST_H_
#define MANTID_TYPES_SPECTRUMDEFINITIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTypes/SpectrumDefinition.h"

using Mantid::SpectrumDefinition;

class SpectrumDefinitionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumDefinitionTest *createSuite() {
    return new SpectrumDefinitionTest();
  }
  static void destroySuite(SpectrumDefinitionTest *suite) { delete suite; }

  void test_size() {
    SpectrumDefinition def;
    TS_ASSERT_EQUALS(def.size(), 0);
  }

  void test_add() {
    SpectrumDefinition def;
    TS_ASSERT_EQUALS(def.size(), 0);
    def.add(42);
    TS_ASSERT_EQUALS(def.size(), 1);
    TS_ASSERT_EQUALS(def[0], (std::pair<size_t, size_t>(42, 0)));
    def.add(24);
    TS_ASSERT_EQUALS(def.size(), 2);
    TS_ASSERT_EQUALS(def[0], (std::pair<size_t, size_t>(24, 0)));
    TS_ASSERT_EQUALS(def[1], (std::pair<size_t, size_t>(42, 0)));
  }

  void test_add_with_time_index() {
    SpectrumDefinition def;
    TS_ASSERT_EQUALS(def.size(), 0);
    def.add(42, 1);
    TS_ASSERT_EQUALS(def.size(), 1);
    TS_ASSERT_EQUALS(def[0], (std::pair<size_t, size_t>(42, 1)));
    def.add(24, 2);
    TS_ASSERT_EQUALS(def.size(), 2);
    TS_ASSERT_EQUALS(def[0], (std::pair<size_t, size_t>(24, 2)));
    TS_ASSERT_EQUALS(def[1], (std::pair<size_t, size_t>(42, 1)));
  }

  void test_uniqueness() {
    SpectrumDefinition def;
    def.add(1);
    def.add(1);
    TS_ASSERT_EQUALS(def.size(), 1);
    def.add(1, 1);
    TS_ASSERT_EQUALS(def.size(), 2);
  }

  void test_elements_are_sorted() {
    SpectrumDefinition def;
    def.add(1, 1);
    def.add(0, 1);
    def.add(1);
    def.add(2);
    def.add(1);
    TS_ASSERT_EQUALS(def[0], (std::pair<size_t, size_t>(0, 1)));
    TS_ASSERT_EQUALS(def[1], (std::pair<size_t, size_t>(1, 0)));
    TS_ASSERT_EQUALS(def[2], (std::pair<size_t, size_t>(1, 1)));
    TS_ASSERT_EQUALS(def[3], (std::pair<size_t, size_t>(2, 0)));
  }

  void test_iterators_empty() {
    SpectrumDefinition def;
    TS_ASSERT_EQUALS(def.begin(), def.end());
    TS_ASSERT_EQUALS(def.cbegin(), def.cend());
  }

  void test_iterators() {
    SpectrumDefinition def;
    def.add(1);
    TS_ASSERT_DIFFERS(def.begin(), def.end());
    TS_ASSERT_DIFFERS(def.cbegin(), def.cend());
    TS_ASSERT_EQUALS(def.begin() + 1, def.end());
    TS_ASSERT_EQUALS(def.cbegin() + 1, def.cend());
    TS_ASSERT_EQUALS(*(def.begin()), (std::pair<size_t, size_t>(1, 0)));
    TS_ASSERT_EQUALS(*(def.cbegin()), (std::pair<size_t, size_t>(1, 0)));
  }
};

#endif /* MANTID_TYPES_SPECTRUMDEFINITIONTEST_H_ */
