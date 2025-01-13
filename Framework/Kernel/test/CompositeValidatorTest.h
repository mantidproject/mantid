// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class CompositeValidatorTest : public CxxTest::TestSuite {
public:
  /** Is valid does an AND of the components */
  void test_isValid() {
    auto val1 = std::make_shared<BoundedValidator<int>>(100, 1000);
    auto val2 = std::make_shared<BoundedValidator<int>>(900, 2000);

    CompositeValidator comp;
    comp.add(val1);
    //
    TS_ASSERT(comp.allowedValues().empty());

    TS_ASSERT_EQUALS(comp.isValid(150), "");
    TS_ASSERT_EQUALS(comp.isValid(950), "");
    TS_ASSERT_DIFFERS(comp.isValid(1200), "");
    comp.add(val2);
    TS_ASSERT_DIFFERS(comp.isValid(150),
                      ""); // This one is now blocked by validator 2
    TS_ASSERT_EQUALS(comp.isValid(950), "");
    TS_ASSERT_DIFFERS(comp.isValid(1200), "");
    //
    TS_ASSERT(comp.allowedValues().empty());

    // Test cloning
    IValidator_sptr comp2 = comp.clone();
    TS_ASSERT(!comp2->isValid(150).empty());
    TS_ASSERT(comp2->isValid(950).empty());

    TS_ASSERT(comp2->allowedValues().empty());
  }
  void test_isListObtained() {
    std::vector<std::string> allowed_val1(3);
    allowed_val1[0] = "a1";
    allowed_val1[1] = "b2";
    allowed_val1[2] = "c";

    auto val1 = std::make_shared<StringListValidator>(allowed_val1);
    CompositeValidator comp;
    comp.add(val1);

    std::vector<std::string> allowed = comp.allowedValues();
    TS_ASSERT_EQUALS(allowed_val1.size(), allowed.size());

    std::vector<std::string> allowed_val2(3);
    allowed_val2[0] = "a2";
    allowed_val2[1] = "b2";
    allowed_val2[2] = "c2";

    auto val2 = std::make_shared<StringListValidator>(allowed_val2);
    comp.add(val2);

    std::vector<std::string> allowed2 = comp.allowedValues();
    TS_ASSERT_EQUALS(1, allowed2.size());
    TS_ASSERT_EQUALS("b2", *(allowed2.begin()));
  }

  void test_Given_TwoValidators_When_CheckIsValid_That_ValidValuesReturnValid() {
    // Arrange
    auto val1 = std::make_shared<BoundedValidator<int>>(1, 50);
    auto val2 = std::make_shared<BoundedValidator<int>>(60, 100);

    CompositeValidator comp(CompositeRelation::OR);
    comp.add(val1);
    comp.add(val2);

    // Assert
    TS_ASSERT_EQUALS(comp.isValid(30), "");  // In range of val1
    TS_ASSERT_EQUALS(comp.isValid(70), "");  // In range of val2
    TS_ASSERT_DIFFERS(comp.isValid(55), ""); // Not in range
  }

  void test_ContainsReturnsTrueIfListContainsType() {
    CompositeValidator comp;
    auto val1 = std::make_shared<BoundedValidator<int>>(1, 50);
    auto val2 = std::make_shared<BoundedValidator<double>>(60, 100);
    comp.add(val1);
    comp.add(val2);

    TS_ASSERT(comp.contains<BoundedValidator<int>>());
    TS_ASSERT(comp.contains<BoundedValidator<double>>());
  }

  void test_ContainsReturnsFalseIfListDoesNotContainType() {
    CompositeValidator comp;
    auto val1 = std::make_shared<BoundedValidator<int>>(1, 50);
    auto val2 = std::make_shared<BoundedValidator<double>>(60, 100);
    comp.add(val1);
    comp.add(val2);

    TS_ASSERT_EQUALS(false, comp.contains<StringListValidator>());
  }
};
