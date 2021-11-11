// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"

#include <cxxtest/TestSuite.h>

using AlgorithmRuntimeProps = MantidQt::API::AlgorithmRuntimeProps;

class AlgorithmRuntimePropsTest : public CxxTest::TestSuite {
public:
  template <typename T> void assert_set_get(const T &val) {
    AlgorithmRuntimeProps props;
    props.setProperty("property", val);
    TS_ASSERT_EQUALS(val, static_cast<T>(props.getProperty("property")));
  }

  void test_set_property_with_type_t() {
    assert_set_get(0);
    assert_set_get(1.123);
    assert_set_get(WorkspaceCreationHelper::createWorkspaceSingleValue(1.0));
  }

  void test_set_property_called_multiple_times() {
    AlgorithmRuntimeProps props;
    const std::string propName = "test";
    props.setProperty(propName, 1);
    TS_ASSERT_EQUALS(1, static_cast<int>(props.getProperty(propName)))

    props.setProperty(propName, 2);
    TS_ASSERT_EQUALS(2, static_cast<int>(props.getProperty(propName)))
  }

  void test_set_property_with_string() {
    AlgorithmRuntimeProps props;
    const std::string expected_value = "a string";
    props.setPropertyValue("test", expected_value);
    TS_ASSERT_EQUALS(expected_value, props.getPropertyValue("test"))
  }
};
