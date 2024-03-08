// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/ValidationUtils.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class ValidationUtilsTest : public CxxTest::TestSuite {
public:
  static ValidationUtilsTest *createSuite() { return new ValidationUtilsTest(); }

  static void destroySuite(ValidationUtilsTest *suite) { delete suite; }

  void setUp() override {
    m_properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
    m_spectraMin = 0u;
    m_spectraMax = 50u;
  }

  void test_when_grouping_method_is_file_and_a_file_is_provided() {
    m_properties->setProperty("GroupingMethod", "File");
    m_properties->setProperty("MapFile", "/path/to/a/grouping/file.map");

    auto const message =
        ValidationUtils::validateGroupingProperties(std::move(m_properties), m_spectraMin, m_spectraMax);

    TS_ASSERT(!message);
  }

  void test_when_grouping_method_is_file_and_a_file_is_not_provided() {
    m_properties->setProperty("GroupingMethod", "File");

    auto const message =
        ValidationUtils::validateGroupingProperties(std::move(m_properties), m_spectraMin, m_spectraMax);

    TS_ASSERT(message);
    TS_ASSERT_EQUALS("Please supply a map file for grouping detectors.", *message);
  }

  void test_when_grouping_method_is_custom_and_a_custom_string_is_provided() {
    m_properties->setProperty("GroupingMethod", "Custom");
    m_properties->setProperty("GroupingString", "0-50");

    auto const message =
        ValidationUtils::validateGroupingProperties(std::move(m_properties), m_spectraMin, m_spectraMax);

    TS_ASSERT(!message);
  }

  void test_when_grouping_method_is_custom_and_a_custom_string_not_provided() {
    m_properties->setProperty("GroupingMethod", "Custom");

    auto const message =
        ValidationUtils::validateGroupingProperties(std::move(m_properties), m_spectraMin, m_spectraMax);

    TS_ASSERT(message);
    TS_ASSERT_EQUALS("Please supply a custom string for grouping detectors.", *message);
  }

  void test_when_grouping_method_is_custom_and_the_custom_string_is_out_of_range() {
    m_properties->setProperty("GroupingMethod", "Custom");
    m_properties->setProperty("GroupingString", "0-100");

    auto const message =
        ValidationUtils::validateGroupingProperties(std::move(m_properties), m_spectraMin, m_spectraMax);

    TS_ASSERT(message);
    TS_ASSERT_EQUALS("Please supply a custom grouping within the correct range.", *message);
  }

  void test_when_grouping_method_is_groups_and_a_valid_number_of_groups_is_provided() {
    m_properties->setProperty("GroupingMethod", "Groups");
    m_properties->setProperty("NGroups", "51");

    auto const message =
        ValidationUtils::validateGroupingProperties(std::move(m_properties), m_spectraMin, m_spectraMax);

    TS_ASSERT(!message);
  }

  void test_when_grouping_method_is_groups_and_the_number_of_groups_is_too_large() {
    m_properties->setProperty("GroupingMethod", "Groups");
    m_properties->setProperty("NGroups", "52");

    auto const message =
        ValidationUtils::validateGroupingProperties(std::move(m_properties), m_spectraMin, m_spectraMax);

    TS_ASSERT(message);
    TS_ASSERT_EQUALS("The number of groups must be less or equal to the number of spectra (51).", *message);
  }

private:
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> m_properties;
  std::size_t m_spectraMin;
  std::size_t m_spectraMax;
};
