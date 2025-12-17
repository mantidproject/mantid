// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyHistory.h"
#include <json/value.h>

using namespace Mantid::Kernel;

namespace {

// Test helper class to allow access to `Property` methods.
class PropertyHelper : public Property {
public:
  PropertyHelper(const std::string &name = "Test") : Property(name, typeid(int)) {}
  PropertyHelper *clone() const override { return new PropertyHelper(*this); }
  std::string value() const override { return "Nothing"; }
  Json::Value valueAsJson() const override { return Json::Value(); }
  std::string setValue(const std::string &) override { return ""; }
  std::string setValueFromJson(const Json::Value &) override { return ""; }
  std::string setValueFromProperty(const Property &) override { return ""; }
  std::string setDataItem(const std::shared_ptr<DataItem> &) override { return ""; }
  bool isDefault() const override { return true; }
  std::string getDefault() const override { return "Is not implemented in this class, should be overriden"; }
  Property &operator+=(Property const *) override { return *this; }
};

class MockPropertySettings : public IPropertySettings {
public:
  using ApplyCallback = std::function<bool(const IPropertyManager *, const std::string &)>;

  bool isEnabled(const IPropertyManager *) const override { return m_isEnabledDefault; }

  bool isVisible(const IPropertyManager *) const override { return m_isVisibleDefault; }

  bool isConditionChanged(const IPropertyManager *, const std::string &) const override {
    return m_isConditionChangedDefault;
  }

  bool applyChanges(const IPropertyManager *algo, const std::string &currentPropName) const override {
    if (m_applyCallback)
      return m_applyCallback(algo, currentPropName);
    return false;
  }

  MockPropertySettings() = default;

  IPropertySettings *clone() const override {
    auto *copy = new MockPropertySettings();

    // Copy configuration state
    copy->m_isEnabledDefault = m_isEnabledDefault;
    copy->m_isVisibleDefault = m_isVisibleDefault;
    copy->m_isConditionChangedDefault = m_isConditionChangedDefault;
    copy->m_applyCallback = m_applyCallback;

    return copy;
  }

  void setIsEnabledReturn(bool v) { m_isEnabledDefault = v; }
  void setIsVisibleReturn(bool v) { m_isVisibleDefault = v; }
  void setIsConditionChangedReturn(bool v) { m_isConditionChangedDefault = v; }
  void setApplyChangesCallback(ApplyCallback cb) { m_applyCallback = std::move(cb); }

private:
  bool m_isEnabledDefault{true};
  bool m_isVisibleDefault{true};
  bool m_isConditionChangedDefault{false};
  ApplyCallback m_applyCallback;
};

} // namespace

class PropertyTest : public CxxTest::TestSuite {
public:
  static PropertyTest *createSuite() { return new PropertyTest(); }
  static void destroySuite(PropertyTest *suite) { delete suite; }

  PropertyTest() { p = std::make_unique<PropertyHelper>(); }

  void testName() { TS_ASSERT(!p->name().compare("Test")); }

  void testEmptyNameNotPermitted() { TS_ASSERT_THROWS(PropertyHelper(""), const std::invalid_argument &); }

  void testDocumentation() {
    auto pp = std::make_unique<PropertyHelper>();
    TS_ASSERT(pp->documentation().empty());
  }

  void testType_info() { TS_ASSERT(typeid(int) == *p->type_info()); }

  void testType() { TS_ASSERT(!p->type().compare("number")); }

  void testisValid() { TS_ASSERT_EQUALS(p->isValid(), ""); }

  void testIsDefault() { TS_ASSERT(p->isDefault()); }

  void testSetDocumentation() {
    const std::string str("Doc comment. This property does something.");
    p->setDocumentation(str);
    TS_ASSERT_EQUALS(p->documentation(), str);

    const std::string str2("A string with no period to be seen");
    // Brief documentation not changed if it's not empty
    p->setDocumentation(str2);
    TS_ASSERT_EQUALS(p->documentation(), str2);

    // Make it empty and see that it will now be changed via setDocumentation()
    p->setDocumentation(str2);
    TS_ASSERT_EQUALS(p->documentation(), str2);
  }

  void testIsValueSerializable() { TS_ASSERT(p->isValueSerializable()) }

  void testAllowedValues() { TS_ASSERT(p->allowedValues().empty()); }

  void testCreateHistory() {
    PropertyHistory history = p->createHistory();
    TS_ASSERT_EQUALS(history.name(), "Test");
    TS_ASSERT_EQUALS(history.value(), "Nothing");
    TS_ASSERT(history.isDefault());
    TS_ASSERT_EQUALS(history.type(), p->type());
    TS_ASSERT_EQUALS(history.direction(), 0);
  }

  void testUnits() {
    auto p2 = std::make_unique<PropertyHelper>();
    // No unit at first
    TS_ASSERT_EQUALS(p2->units(), "");
    p2->setUnits("furlongs/fortnight");
    TS_ASSERT_EQUALS(p2->units(), "furlongs/fortnight");
  }

  void testRemember() {
    auto p3 = std::make_unique<PropertyHelper>();
    TS_ASSERT(p3->remember());
    p3->setRemember(false);
    TS_ASSERT(!p3->remember());
    p3->setRemember(true);
    TS_ASSERT(p3->remember());
  }

  void testDisableReplaceWSButton() {
    auto p = std::make_unique<PropertyHelper>();
    TS_ASSERT(!p->disableReplaceWSButton());
    p->setDisableReplaceWSButton(true);
    TS_ASSERT(p->disableReplaceWSButton());
    p->setDisableReplaceWSButton(false);
    TS_ASSERT(!p->disableReplaceWSButton());
  }

  /// Verify that `Property::getSettings()` with no attached `IPropertySettings`
  /// returns an empty vector.
  void test_getSettings_empty() {
    auto p = std::make_unique<PropertyHelper>();
    TS_ASSERT(p->getSettings().empty());
  }

  /// Verify that `Property::getSettings()` with multiple attached `IPropertySettings`
  /// returns an vector of settings.
  void test_getSettings_multiple() {
    auto p = std::make_unique<PropertyHelper>();
    TS_ASSERT(p->getSettings().empty());

    const size_t N_settings = 5;
    for (size_t n = 0; n < N_settings; ++n)
      p->setSettings(std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    TS_ASSERT_EQUALS(p->getSettings().size(), N_settings);
  }

  /// Verify that `Property::clearSettings()` with multiple attached `IPropertySettings`
  /// clears the settings vector.
  void test_clearSettings() {
    auto p = std::make_unique<PropertyHelper>();
    const size_t N_settings = 5;
    for (size_t n = 0; n < N_settings; ++n)
      p->setSettings(std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    TS_ASSERT_EQUALS(p->getSettings().size(), N_settings);

    p->clearSettings();
    TS_ASSERT(p->getSettings().empty());
  }

private:
  std::unique_ptr<Property> p;
};
