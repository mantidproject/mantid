// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerOwner.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class EnabledWhenPropertyTest : public CxxTest::TestSuite {
public:
  void test_when_IS_NOT_DEFAULT() {
    PropertyManagerOwner alg;
    // Start with a regular property
    alg.declareProperty(m_propertyOneName, 123);

    // Make a property with its validator. Will be enabled when that other one
    // is NOT the default
    auto enabledWhenNotDefault = [this] {
      return std::make_unique<EnabledWhenProperty>(m_propertyOneName.c_str(), IS_NOT_DEFAULT);
    };
    alg.declareProperty(m_resultPropName, 456);
    alg.setPropertySettings(m_resultPropName, enabledWhenNotDefault());

    Property *prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);

    TSM_ASSERT("Property always returns visible.", prop->getSettings()->isVisible(&alg))
    TSM_ASSERT("Property always returns valid.", prop->isValid().empty())

    TSM_ASSERT("Starts off NOT enabled", !prop->getSettings()->isEnabled(&alg));
    // Change the property so it is no longer default
    alg.setProperty(m_propertyOneName, 234);
    TSM_ASSERT("Becomes enabled when another property has been changed", prop->getSettings()->isEnabled(&alg));

    alg.declareProperty(m_propertyTwoName, 456);
    alg.setPropertySettings(m_propertyTwoName, enabledWhenNotDefault());
    prop = alg.getPointerToProperty(m_propertyTwoName);
    TSM_ASSERT("Starts off enabled", prop->getSettings()->isEnabled(&alg));
    alg.setProperty(m_propertyOneName, 123);
    TSM_ASSERT("Goes back to disabled", !prop->getSettings()->isEnabled(&alg));
  }

  void test_when_IS_DEFAULT() {
    PropertyManagerOwner alg;
    alg.declareProperty(m_propertyOneName, 123);
    // Make a property with its validator. Will be enabled when that other one
    // is the default
    alg.declareProperty(m_resultPropName, 456);
    alg.setPropertySettings(m_resultPropName,
                            std::make_unique<EnabledWhenProperty>(m_propertyOneName.c_str(), IS_DEFAULT));
    Property *prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    if (!prop)
      return;
    TSM_ASSERT("Starts off enabled", prop->getSettings()->isEnabled(&alg));
    alg.setProperty(m_propertyOneName, -1);
    TSM_ASSERT("Becomes disabled when another property has been changed", !prop->getSettings()->isEnabled(&alg));
  }

  void test_when_IS_EQUAL_TO() {
    PropertyManagerOwner alg;
    alg.declareProperty(m_propertyOneName, 123);
    alg.declareProperty(m_resultPropName, 456);
    alg.setPropertySettings(m_resultPropName,
                            std::make_unique<EnabledWhenProperty>(m_propertyOneName.c_str(), IS_EQUAL_TO, "234"));
    Property *prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    if (!prop)
      return;
    TSM_ASSERT("Starts off disabled", !prop->getSettings()->isEnabled(&alg));
    alg.setProperty(m_propertyOneName, 234);
    TSM_ASSERT("Becomes enabled when the other property is equal to the given string",
               prop->getSettings()->isEnabled(&alg));
  }

  void test_when_IS_NOT_EQUAL_TO() {
    PropertyManagerOwner alg;
    alg.declareProperty(m_propertyOneName, 123);
    alg.declareProperty(m_resultPropName, 456);
    alg.setPropertySettings(m_resultPropName,
                            std::make_unique<EnabledWhenProperty>(m_propertyOneName.c_str(), IS_NOT_EQUAL_TO, "234"));
    Property *prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    if (!prop)
      return;
    TSM_ASSERT("Starts off enabled", prop->getSettings()->isEnabled(&alg));
    alg.setProperty(m_propertyOneName, 234);
    TSM_ASSERT("Becomes disabled when the other property is equal to the given string",
               !prop->getSettings()->isEnabled(&alg));
  }

  void test_combination_AND() {
    // Setup with same value first
    auto alg = setupCombinationTest(AND, true);
    const auto prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    const auto propSettings = prop->getSettings();

    // AND should return true first
    TS_ASSERT(propSettings->isEnabled(&alg));

    // Now set a different value - should be disabled
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(!propSettings->isEnabled(&alg));
  }

  void test_combination_OR() {
    // First check with both set to the true value
    auto alg = setupCombinationTest(OR, true);
    const auto prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    const auto propSettings = prop->getSettings();

    // OR should return true for both values on
    TS_ASSERT(propSettings->isEnabled(&alg));

    // Set property one to false condition and check OR is still true
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyTrueValue);
    TS_ASSERT(propSettings->isEnabled(&alg));

    // Set property two to false condition and check OR is still true
    alg.setPropertyValue(m_propertyOneName, m_propertyTrueValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(propSettings->isEnabled(&alg));

    // Check the with both set to false the OR returns false
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(!propSettings->isEnabled(&alg));
  }

  void test_combination_XOR() {
    auto alg = setupCombinationTest(XOR, true);
    const auto prop = alg.getPointerToProperty(m_resultPropName);
    TS_ASSERT(prop);
    const auto propSettings = prop->getSettings();

    // With both set to the same value this should return false
    TS_ASSERT(!propSettings->isEnabled(&alg));

    // Set property one to false and two to true so returns true
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyTrueValue);
    TS_ASSERT(propSettings->isEnabled(&alg));

    // Set property one to true and one to false so returns true
    alg.setPropertyValue(m_propertyOneName, m_propertyTrueValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(propSettings->isEnabled(&alg));

    // Check with both set false it returns false
    alg.setPropertyValue(m_propertyOneName, m_propertyFalseValue);
    alg.setPropertyValue(m_propertyTwoName, m_propertyFalseValue);
    TS_ASSERT(!propSettings->isEnabled(&alg));
  }

private:
  const std::string m_propertyTrueValue = "testTrue";
  const std::string m_propertyFalseValue = "testFalse";
  const std::string m_resultValue = "Result";

  const std::string m_propertyOneName = "PropOne";
  const std::string m_propertyTwoName = "PropTwo";
  const std::string m_resultPropName = "ResultProp";

  PropertyManagerOwner setupCombinationTest(eLogicOperator logicOperation, bool secondPropertyIsTrue) {
    auto propOne = getEnabledWhenProp(m_propertyOneName, IS_EQUAL_TO, m_propertyTrueValue);
    auto propTwo = getEnabledWhenProp(m_propertyTwoName, IS_EQUAL_TO, m_propertyTrueValue);
    auto combination = getCombinationProperty(std::move(propOne), std::move(propTwo), logicOperation);
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

  std::unique_ptr<EnabledWhenProperty> getEnabledWhenProp(const std::string &propName, ePropertyCriterion criterion,
                                                          const std::string &value = "") {
    if (value.length() == 0) {
      return std::make_unique<EnabledWhenProperty>(propName, criterion);
    } else {
      return std::make_unique<EnabledWhenProperty>(propName, criterion, value);
    }
  }

  using EnabledPropPtr = std::unique_ptr<EnabledWhenProperty>;
  std::unique_ptr<IPropertySettings> getCombinationProperty(EnabledPropPtr &&condOne, EnabledPropPtr &&condTwo,
                                                            eLogicOperator logicalOperator) {
    return std::make_unique<EnabledWhenProperty>(std::move(condOne), std::move(condTwo), logicalOperator);
  }
};
