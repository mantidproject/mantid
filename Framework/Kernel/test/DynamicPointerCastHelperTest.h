// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DynamicPointerCastHelper.h"

#include <cxxtest/TestSuite.h>

#include <string>

using namespace Mantid::Kernel::DynamicPointerCastHelper;

class DynamicPointerCastHelperTest : public CxxTest::TestSuite {

public:
  void test_correctCast() {
    auto derivedClass = std::make_shared<EmptyDerivedClass>();
    std::shared_ptr<EmptyBaseClass> baseClass =
        dynamicPointerCastWithCheck<EmptyBaseClass, EmptyDerivedClass>(derivedClass);
  }

  void test_incorrectCast() {
    const auto errorString = "Oops";
    std::shared_ptr<EmptyDerivedClass> nullDerivedPtr;
    TS_ASSERT_THROWS_EQUALS(convertDerivedToBaseClass(nullDerivedPtr, errorString), const std::invalid_argument &e,
                            std::string(e.what()), errorString);
  }

  class EmptyBaseClass {};

  class EmptyDerivedClass : public EmptyBaseClass {};

private:
  std::shared_ptr<EmptyBaseClass> convertDerivedToBaseClass(std::shared_ptr<EmptyDerivedClass> derived,
                                                            const std::string &errorMsg = "") {
    return dynamicPointerCastWithCheck<EmptyBaseClass, EmptyDerivedClass>(derived, errorMsg);
  }
};
