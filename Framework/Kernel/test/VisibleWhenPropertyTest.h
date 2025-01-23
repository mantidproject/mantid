// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/VisibleWhenProperty.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class VisibleWhenPropertyTest : public CxxTest::TestSuite {
public:
  void test_when_IS_NOT_DEFAULT() {
    PropertyManagerOwner alg;
    // Start with a regular property
    alg.declareProperty("MyIntProp", 123);

    // Make a property with its validator. Will be Visible when that other one
    // is NOT the default
    alg.declareProperty("MyValidatorProp", 456);
    alg.setPropertySettings("MyValidatorProp", std::make_unique<VisibleWhenProperty>("MyIntProp", IS_NOT_DEFAULT));

    Property *prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop);
    if (!prop)
      return;
    TSM_ASSERT("Property always returns enabled.", prop->getSettings()->isEnabled(&alg))
    TSM_ASSERT("Property always returns valid.", prop->isValid().empty())

    TSM_ASSERT("Starts off NOT Visible", !prop->getSettings()->isVisible(&alg));
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT("Becomes visible when another property has been changed", prop->getSettings()->isVisible(&alg));

    alg.declareProperty("MySecondValidatorProp", 456);
    alg.setPropertySettings("MySecondValidatorProp",
                            std::make_unique<VisibleWhenProperty>("MyIntProp", IS_NOT_DEFAULT));
    prop = alg.getPointerToProperty("MySecondValidatorProp");
    TSM_ASSERT("Starts off visible", prop->getSettings()->isVisible(&alg));
    alg.setProperty("MyIntProp", 123);
    TSM_ASSERT("Goes back to not visible", !prop->getSettings()->isVisible(&alg));
  }

  void test_when_IS_DEFAULT() {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    // Make a property with its validator. Will be Visible when that other one
    // is the default
    alg.declareProperty("MyValidatorProp", 456);
    alg.setPropertySettings("MyValidatorProp", std::make_unique<VisibleWhenProperty>("MyIntProp", IS_DEFAULT));
    Property *prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop);
    if (!prop)
      return;
    TSM_ASSERT("Starts off visible", prop->getSettings()->isVisible(&alg));
    alg.setProperty("MyIntProp", -1);
    TSM_ASSERT("Becomes not visible when another property has been changed", !prop->getSettings()->isVisible(&alg));
  }

  void test_when_IS_EQUAL_TO() {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    alg.declareProperty("MyValidatorProp", 456);
    alg.setPropertySettings("MyValidatorProp", std::make_unique<VisibleWhenProperty>("MyIntProp", IS_EQUAL_TO, "234"));
    Property *prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop);
    if (!prop)
      return;
    TSM_ASSERT("Starts off not visible", !prop->getSettings()->isVisible(&alg));
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT("Becomes visible when the other property is equal to the given string",
               prop->getSettings()->isVisible(&alg));
  }

  void test_when_IS_NOT_EQUAL_TO() {
    PropertyManagerOwner alg;
    alg.declareProperty("MyIntProp", 123);
    alg.declareProperty("MyValidatorProp", 456);
    alg.setPropertySettings("MyValidatorProp",
                            std::make_unique<VisibleWhenProperty>("MyIntProp", IS_NOT_EQUAL_TO, "234"));
    Property *prop = alg.getPointerToProperty("MyValidatorProp");
    TS_ASSERT(prop);
    if (!prop)
      return;
    TSM_ASSERT("Starts off not visible", prop->getSettings()->isVisible(&alg));
    alg.setProperty("MyIntProp", 234);
    TSM_ASSERT("Becomes visible when the other property is equal to the given string",
               !prop->getSettings()->isVisible(&alg));
  }

  void test_combination_AND() {
    // Setup with same value first
    auto alg = setupCombinationTest(AND, true);
    const auto prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    const auto propSettings = prop->getSettings();

    // AND should return true first
    TS_ASSERT(propSettings->isVisible(&alg));

    // Now set a different value - should be disabled
    alg.setPropertyValue(m_propertyOneName, m_propertyTrueValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(!propSettings->isVisible(&alg));
  }

  void test_combination_OR() {
    // First check with both set to the true value
    auto alg = setupCombinationTest(OR, true);
    const auto prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    const auto propSettings = prop->getSettings();

    // OR should return true for both values on
    TS_ASSERT(propSettings->isVisible(&alg));

    // Set property one to false condition and check OR is still true
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyTrueValue);
    TS_ASSERT(propSettings->isVisible(&alg));

    // Set property two to false condition and check OR is still true
    alg.setPropertyValue(m_propertyOneName, m_propertyTrueValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(propSettings->isVisible(&alg));

    // Check the with both set to false the OR returns false
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(!propSettings->isVisible(&alg));
  }

  void test_combination_XOR() {
    auto alg = setupCombinationTest(XOR, true);
    const auto prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    const auto propSettings = prop->getSettings();

    // With both set to the same value this should return false
    TS_ASSERT(!propSettings->isVisible(&alg));

    // Set property one to false and two to true so returns true
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyTrueValue);
    TS_ASSERT(propSettings->isVisible(&alg));

    // Set property one to true and one to false so returns true
    alg.setPropertyValue(m_propertyOneName, m_propertyTrueValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(propSettings->isVisible(&alg));

    // Check with both set false it returns false
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(!propSettings->isVisible(&alg));
  }

private:
  const std::string m_propertyTrueValue = "testTrue";
  const std::string m_propertyFalseValue = "testFalse";
  const std::string m_resultValue = "Result";

  const std::string m_propertyOneName = "PropOne";
  const std::string m_propertyTwoName = "PropTwo";
  const std::string m_resultPropName = "ResultProp";

  PropertyManagerOwner setupCombinationTest(eLogicOperator logicOperation, bool secondPropertyIsTrue) {
    auto propOne = getVisibleWhenProp(m_propertyOneName, IS_EQUAL_TO, m_propertyTrueValue);
    auto propTwo = getVisibleWhenProp(m_propertyTwoName, IS_EQUAL_TO, m_propertyTrueValue);
    auto combination = getCombinationProperty(propOne, propTwo, logicOperation);
    // Set both to the same value to check
    PropertyManagerOwner alg;
    alg.declareProperty(m_propertyOneName, m_propertyTrueValue);
    if (secondPropertyIsTrue) {
      alg.declareProperty(m_propertyTwoName, m_propertyTrueValue);
    } else {
      alg.declareProperty(m_propertyTwoName, m_propertyFalseValue);
    }
    alg.declareProperty(m_resultPropName, m_resultValue);
    alg.setPropertySettings(m_resultPropName, std::move(combination));
    return alg;
  }

  VisibleWhenProperty getVisibleWhenProp(const std::string &propName, ePropertyCriterion criterion,
                                         const std::string &value = "") {
    if (value.length() == 0) {
      return VisibleWhenProperty(propName, criterion);
    } else {
      return VisibleWhenProperty(propName, criterion, value);
    }
  }

  // Check the copy constructor works instead of std::make_unique
  std::unique_ptr<IPropertySettings> getCombinationProperty(const VisibleWhenProperty &condOne,
                                                            const VisibleWhenProperty &condTwo,
                                                            eLogicOperator logicalOperator) {
    return std::make_unique<VisibleWhenProperty>(condOne, condTwo, logicalOperator);
  }
};
