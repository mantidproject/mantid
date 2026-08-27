// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/DataSelector.h"

#include <cxxtest/TestSuite.h>

using MantidQt::MantidWidgets::DataSelector;

class DataSelectorTest : public CxxTest::TestSuite {
public:
  static DataSelectorTest *createSuite() { return new DataSelectorTest(); }
  static void destroySuite(DataSelectorTest *suite) { delete suite; }

  void test_setLoadProperty_with_string_literal_stores_string_not_bool() {
    DataSelector selector;

    // Call setLoadProperty with a string literal (const char*)
    // This should resolve to the const char* overload which forwards to the std::string overload,
    // NOT the bool overload (which would convert the pointer to bool)
    selector.setLoadProperty("Unit", "DeltaE");

    // Verify the property was stored as the string "DeltaE", not as a bool
    TS_ASSERT_EQUALS(selector.m_loadProperties.getPropertyValue("Unit"), "DeltaE");
  }

  void test_setLoadProperty_with_std_string_stores_string() {
    DataSelector selector;

    // Call setLoadProperty with an explicit std::string
    selector.setLoadProperty("Unit", std::string("TOF"));

    // Verify the property was stored as the string "TOF"
    TS_ASSERT_EQUALS(selector.m_loadProperties.getPropertyValue("Unit"), "TOF");
  }

  void test_setLoadProperty_with_bool_stores_bool() {
    DataSelector selector;

    // Call setLoadProperty with a bool
    selector.setLoadProperty("LoadMonitors", true);

    // Verify the property was stored as a bool (which PropertyManager stores as "1" for true)
    TS_ASSERT_EQUALS(selector.m_loadProperties.getPropertyValue("LoadMonitors"), "1");

    // Also test false
    selector.setLoadProperty("LoadMonitors", false);
    TS_ASSERT_EQUALS(selector.m_loadProperties.getPropertyValue("LoadMonitors"), "0");
  }

  void test_overload_resolution_prefers_const_char_ptr_for_literals() {
    DataSelector selector;

    // These calls should all resolve to the const char* overload (which forwards to std::string),
    // not the bool overload. The key is that these compile without ambiguity and don't
    // accidentally convert the string literal pointer to bool.
    selector.setLoadProperty("Unit", "DeltaE");
    selector.setLoadProperty("OutputWorkspace", "output_ws");
    selector.setLoadProperty("Property1", "Value1");

    // Verify all were stored as strings, not converted to bool
    TS_ASSERT_EQUALS(selector.m_loadProperties.getPropertyValue("Unit"), "DeltaE");
    TS_ASSERT_EQUALS(selector.m_loadProperties.getPropertyValue("OutputWorkspace"), "output_ws");
    TS_ASSERT_EQUALS(selector.m_loadProperties.getPropertyValue("Property1"), "Value1");
  }
};
