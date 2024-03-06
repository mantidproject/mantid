// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidJson/Json.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/scoped_ptr.hpp>
#include <json/json.h>

using namespace Mantid::Kernel;

namespace {
class MockNonSerializableProperty : public PropertyWithValue<int> {
public:
  MockNonSerializableProperty(const std::string &name, const int defaultValue)
      : PropertyWithValue<int>(name, defaultValue, Direction::InOut) {}
  bool isValueSerializable() const override { return false; }
  using PropertyWithValue<int>::operator=;
};

/// Create the test source property
std::unique_ptr<Mantid::Kernel::TimeSeriesProperty<double>> createTestSeries(const std::string &name) {
  auto source = std::make_unique<Mantid::Kernel::TimeSeriesProperty<double>>(name);
  source->addValue("2007-11-30T16:17:00", 1);
  source->addValue("2007-11-30T16:17:10", 2);
  source->addValue("2007-11-30T16:17:20", 3);
  source->addValue("2007-11-30T16:17:30", 4);
  source->addValue("2007-11-30T16:17:40", 5);
  return source;
}

/// Create test filter
Mantid::Kernel::TimeSeriesProperty<bool> *createTestFilter() {
  auto filter = new Mantid::Kernel::TimeSeriesProperty<bool>("filter");
  filter->addValue("2007-11-30T16:16:50", false);
  filter->addValue("2007-11-30T16:17:25", true);
  filter->addValue("2007-11-30T16:17:39", false);
  return filter;
}
} // namespace

class PropertyManagerHelper : public PropertyManager {
public:
  PropertyManagerHelper() : PropertyManager() {}

  using PropertyManager::declareProperty;
  using PropertyManager::getPointerToProperty;
  using PropertyManager::setProperty;
};

class PropertyManagerTest : public CxxTest::TestSuite {
public:
  static PropertyManagerTest *createSuite() { return new PropertyManagerTest; }

  static void destroySuite(PropertyManagerTest *suite) { return delete suite; }

public:
  void setUp() override {
    manager = std::make_unique<PropertyManagerHelper>();
    auto p = std::make_unique<PropertyWithValue<int>>("aProp", 1);
    manager->declareProperty(std::move(p));
    manager->declareProperty("anotherProp", 1.11);
    manager->declareProperty("yetAnotherProp", "itsValue");
  }

  void testStaticMethods() {
    const std::string expected_suffix = "_invalid_values";
    TSM_ASSERT_EQUALS("The expected suffix does not match to the line above this assert",
                      PropertyManager::INVALID_VALUES_SUFFIX, expected_suffix);

    const std::string test_log_name = "testLog";
    TS_ASSERT(!PropertyManager::isAnInvalidValuesFilterLog(test_log_name));
    TS_ASSERT_EQUALS(PropertyManager::getInvalidValuesFilterLogName(test_log_name), test_log_name + expected_suffix);

    TS_ASSERT(
        PropertyManager::isAnInvalidValuesFilterLog(PropertyManager::getInvalidValuesFilterLogName(test_log_name)));

    // not a valid invalid values log
    TS_ASSERT_EQUALS(PropertyManager::getLogNameFromInvalidValuesFilter(test_log_name), "");
    // A valid invalid values log
    TS_ASSERT_EQUALS(PropertyManager::getLogNameFromInvalidValuesFilter(
                         PropertyManager::getInvalidValuesFilterLogName(test_log_name)),
                     test_log_name);
  }

  void testConstructor() {
    PropertyManagerHelper mgr;
    std::vector<Property *> props = mgr.getProperties();
    TS_ASSERT(props.empty());
  }

  void testFilterByLogConvertsTimeSeriesToFilteredTimeSeries() {
    PropertyManagerHelper manager;
    manager.declareProperty(createTestSeries("log1"));
    manager.declareProperty(createTestSeries("log2"));
    manager.declareProperty(createTestSeries("log3"));

    auto filter = std::make_unique<LogFilter>(*createTestFilter());
    manager.filterByProperty(filter.get());

    TS_ASSERT_EQUALS(manager.propertyCount(), 3);

    const std::vector<Property *> &props = manager.getProperties();
    for (auto iter : props) {
      Property *prop = iter;
      std::string msg = "Property " + prop->name() + " has not been changed to a FilteredTimeSeries";
      auto filteredProp = dynamic_cast<FilteredTimeSeriesProperty<double> *>(iter);
      TSM_ASSERT(msg, filteredProp);
    }

    // Also check the single getter
    Property *log2 = manager.getProperty("log2");
    auto filteredProp = dynamic_cast<FilteredTimeSeriesProperty<double> *>(log2);
    TSM_ASSERT("getProperty has not returned a FilteredProperty as expected", filteredProp);
  }

  void testCopyConstructor() {
    PropertyManagerHelper mgr1;
    mgr1.declareProperty("aProp", 10);
    PropertyManagerHelper mgr2 = mgr1;
    const std::vector<Property *> &props1 = mgr1.getProperties();
    const std::vector<Property *> &props2 = mgr2.getProperties();
    TS_ASSERT_EQUALS(props1.size(), props2.size());
    TS_ASSERT_DIFFERS(&props1[0], &props2[0]);
    TS_ASSERT_EQUALS(props1[0]->name(), props2[0]->name());
    TS_ASSERT_EQUALS(props1[0]->value(), props2[0]->value());
  }

  void testCopyAssignment() {
    PropertyManagerHelper mgr1;
    mgr1.declareProperty("aProp", 10);
    PropertyManagerHelper mgr2;
    mgr2 = mgr1;
    const std::vector<Property *> &props1 = mgr1.getProperties();
    const std::vector<Property *> &props2 = mgr2.getProperties();
    TS_ASSERT_EQUALS(props1.size(), props2.size());
    TS_ASSERT_DIFFERS(&props1[0], &props2[0]);
    TS_ASSERT_EQUALS(props1[0]->name(), props2[0]->name());
    TS_ASSERT_EQUALS(props1[0]->value(), props2[0]->value());
  }

  void testdeclareProperty_pointer() {
    PropertyManagerHelper mgr;
    std::unique_ptr<Property> p = std::make_unique<PropertyWithValue<double>>("myProp", 9.99);
    auto copy = std::unique_ptr<Property>(p->clone());
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(std::move(p)));
    TS_ASSERT(mgr.existsProperty(copy->name()));
    // Confirm that the first 4 characters of the string are the same

    // Note that some versions of boost::lexical_cast > 1.34 give a string such
    // as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however
    // does
    // still give the correct 9.99.

    TS_ASSERT_EQUALS(mgr.getPropertyValue("myProp").substr(0, 4), std::string("9.99"));

    TS_ASSERT_THROWS(mgr.declareProperty(std::move(copy)), const Exception::ExistsError &);
    TS_ASSERT_THROWS(mgr.declareProperty(std::make_unique<PropertyWithValue<int>>("", 0)),
                     const std::invalid_argument &);
    mgr.declareProperty(std::make_unique<PropertyWithValue<int>>("GoodIntProp", 1), "Test doc");
    TS_ASSERT_EQUALS(mgr.getPointerToProperty("GoodIntProp")->documentation(), "Test doc");
  }

  void testdeclareProperty_int() {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("myProp", 1));
    TS_ASSERT(!mgr.getPropertyValue("myProp").compare("1"));
    TS_ASSERT_THROWS(mgr.declareProperty("MYPROP", 5), const Exception::ExistsError &);
    TS_ASSERT_THROWS(mgr.declareProperty("", 5), const std::invalid_argument &);
  }

  void testdeclareProperty_double() {
    PropertyManagerHelper mgr;
    std::shared_ptr<BoundedValidator<double>> v = std::make_shared<BoundedValidator<double>>(1, 5);
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("myProp", 9.99, v));
    // Note that some versions of boost::lexical_cast > 1.34 give a string such
    // as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however
    // does
    // still give the correct 9.99.
    TS_ASSERT_EQUALS(mgr.getPropertyValue("myProp").substr(0, 4), std::string("9.99"));
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("withDoc", 4.4, v->clone(), "Test doc doub"));
    TS_ASSERT_EQUALS(mgr.getPointerToProperty("withDoc")->documentation(), "Test doc doub");
    TS_ASSERT_THROWS(mgr.declareProperty("MYPROP", 5.5), const Exception::ExistsError &);
    TS_ASSERT_THROWS(mgr.declareProperty("", 5.5), const std::invalid_argument &);
  }

  void testdeclareProperty_string() {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING(
        mgr.declareProperty("myProp", "theValue", std::make_shared<MandatoryValidator<std::string>>(), "hello"));
    TS_ASSERT_EQUALS(mgr.getPropertyValue("myProp"), "theValue");
    Property *p = nullptr;
    TS_ASSERT_THROWS_NOTHING(p = mgr.getProperty("myProp"));
    TS_ASSERT_EQUALS(p->documentation(), "hello");

    TS_ASSERT_THROWS(mgr.declareProperty("MYPROP", "aValue"), const Exception::ExistsError &);
    TS_ASSERT_THROWS(mgr.declareProperty("", "aValue"), const std::invalid_argument &);
  }

  void testDeclareOrReplaceProperty() {
    PropertyManagerHelper mgr;
    mgr.declareProperty("StringProp", "theValue");
    mgr.declareProperty("IntProp", 5);
    mgr.declareProperty("DoubleProp", 2);

    TS_ASSERT_THROWS_NOTHING(mgr.declareOrReplaceProperty(std::make_unique<MockNonSerializableProperty>("IntProp", 2)));
    // it should be replace in the same position
    const auto &properties = mgr.getProperties();
    auto intPropIter =
        std::find_if(std::begin(properties), std::end(properties), [](Property *p) { return p->name() == "IntProp"; });
    TS_ASSERT_EQUALS(1, std::distance(std::begin(properties), intPropIter));
    auto typedProp = dynamic_cast<MockNonSerializableProperty *>(*intPropIter);
    TSM_ASSERT("Expected a int type property", typedProp);
    TS_ASSERT_EQUALS(2, (*typedProp)());
  }

  void testSetProperties() {
    PropertyManagerHelper mgr;
    mgr.declareProperty("APROP", 1);
    mgr.declareProperty("anotherProp", 1.0);

    std::string jsonString = R"({"APROP":"15","anotherProp":"1.3"}\n)";
    TS_ASSERT_THROWS_NOTHING(mgr.setProperties(jsonString));
    TS_ASSERT_EQUALS(mgr.getPropertyValue("APROP"), "15");
    TS_ASSERT_EQUALS(mgr.getPropertyValue("anotherProp"), "1.3");
  }

  void testSetProperties_complicatedValueString() {
    PropertyManagerHelper mgr;
    mgr.declareProperty("APROP", "1");
    mgr.declareProperty("anotherProp", "1");
    std::string jsonString = R"({"APROP":"equation=12+3","anotherProp":"1.3,2.5"}\n)";
    TS_ASSERT_THROWS_NOTHING(mgr.setProperties(jsonString));
    TS_ASSERT_EQUALS(mgr.getPropertyValue("APROP"), "equation=12+3");
    TS_ASSERT_EQUALS(mgr.getPropertyValue("anotherProp"), "1.3,2.5");
  }

  void testSetProperties_complicatedValueString2() {
    PropertyManagerHelper mgr;
    mgr.declareProperty("APROP", "1");
    mgr.declareProperty("anotherProp", "1");

    const std::string jsonString = R"({"APROP":"equation = 12 + 3","anotherProp":"-1.3, +2.5"}\n)";
    TS_ASSERT_THROWS_NOTHING(mgr.setProperties(jsonString));
    TS_ASSERT_EQUALS(mgr.getPropertyValue("APROP"), "equation = 12 + 3");
    TS_ASSERT_EQUALS(mgr.getPropertyValue("anotherProp"), "-1.3, +2.5");
  }

  void testSetProperties_arrayValueString() {
    PropertyManagerHelper mgr;
    mgr.declareProperty(std::make_unique<ArrayProperty<double>>("ArrayProp"));

    const std::string jsonString = R"({"ArrayProp":"10,12,23"})";
    TS_ASSERT_THROWS_NOTHING(mgr.setProperties(jsonString));
    TS_ASSERT_EQUALS(mgr.getPropertyValue("ArrayProp"), "10,12,23");
  }

  void testSetPropertiesWithoutPriorDeclare() {
    PropertyManagerHelper mgr;

    const std::string jsonString = R"({"APROP":"equation=12+3","anotherProp":"1.3,2.5"})";
    std::unordered_set<std::string> ignored;
    const bool createMissing{true};
    TS_ASSERT_THROWS_NOTHING(mgr.setProperties(jsonString, ignored, createMissing));
    TS_ASSERT_EQUALS("equation=12+3", static_cast<std::string>(mgr.getProperty("APROP")));
    TS_ASSERT_EQUALS("1.3,2.5", static_cast<std::string>(mgr.getProperty("anotherProp")));
  }

  void testSetPropertyValue() {
    manager->setPropertyValue("APROP", "10");
    TS_ASSERT(!manager->getPropertyValue("aProp").compare("10"));
    manager->setPropertyValue("aProp", "1");
    TS_ASSERT_THROWS(manager->setPropertyValue("fhfjsdf", "0"), const Exception::NotFoundError &);
  }

  void testSetProperty() {
    TS_ASSERT_THROWS_NOTHING(manager->setProperty("AProp", 5));
    TS_ASSERT_THROWS(manager->setProperty("wefhui", 5), const Exception::NotFoundError &);
    TS_ASSERT_THROWS(manager->setProperty("APROP", 5.55), const std::invalid_argument &);
    TS_ASSERT_THROWS(manager->setProperty("APROP", "value"), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(manager->setProperty("AProp", 1));
  }

  void testSetStringProperty() {
    // Make sure we can handle std::strings as well as const char *.
    TS_ASSERT_THROWS_NOTHING(manager->setProperty("yetAnotherProp", "aValue"));
    std::string aValue("aValue");
    TS_ASSERT_THROWS_NOTHING(manager->setProperty("yetAnotherProp", aValue));
  }

  void testExistsProperty() {
    auto p = std::make_unique<PropertyWithValue<int>>("sjfudh", 0);
    TS_ASSERT(!manager->existsProperty(p->name()));
    TS_ASSERT(manager->existsProperty("APROP"));
  }

  void testGetPropertyNames() {
    std::vector<std::string> expected{"aProp", "anotherProp", "yetAnotherProp"};
    TS_ASSERT_EQUALS(std::move(expected), manager->getDeclaredPropertyNames());
  }

  void testValidateProperties() {
    TS_ASSERT(manager->validateProperties());
    PropertyManagerHelper mgr;
    mgr.declareProperty("someProp", "", std::make_shared<MandatoryValidator<std::string>>());
    TS_ASSERT(!mgr.validateProperties());
  }

  void testPropertyCount() {
    PropertyManagerHelper mgr;
    TS_ASSERT_EQUALS(mgr.propertyCount(), 0);
    const std::string name("TestProperty");
    mgr.declareProperty(name, 10.0);
    TS_ASSERT_EQUALS(mgr.propertyCount(), 1);
    mgr.removeProperty(name);
    TS_ASSERT_EQUALS(mgr.propertyCount(), 0);
  }

  void testGetPropertyValue() {
    TS_ASSERT(!manager->getPropertyValue("APROP").compare("1"));
    TS_ASSERT_THROWS(manager->getPropertyValue("sdfshdu"), const Exception::NotFoundError &);
  }

  void testGetProperty() {
    Property *p = manager->getProperty("APROP");
    TS_ASSERT(p);
    TS_ASSERT(!p->name().compare("aProp"));
    TS_ASSERT(!p->value().compare("1"));
    TS_ASSERT(!p->documentation().compare(""));
    TS_ASSERT(typeid(int) == *p->type_info());

    TS_ASSERT_THROWS(p = manager->getProperty("werhui"), const Exception::NotFoundError &);

    int i(0);
    TS_ASSERT_THROWS_NOTHING(i = manager->getProperty("aprop"));
    TS_ASSERT_EQUALS(i, 1);
    double dd(0.0);
    TS_ASSERT_THROWS(dd = manager->getProperty("aprop"), const std::runtime_error &);
    TS_ASSERT_EQUALS(dd, 0.0); // If dd is bot used you get a compiler warning
    std::string s = manager->getProperty("aprop");
    TS_ASSERT(!s.compare("1"));
    double d(0.0);
    TS_ASSERT_THROWS_NOTHING(d = manager->getProperty("anotherProp"));
    TS_ASSERT_EQUALS(d, 1.11);
    int ii(0);
    TS_ASSERT_THROWS(ii = manager->getProperty("anotherprop"), const std::runtime_error &);
    TS_ASSERT_EQUALS(ii, 0); // Compiler warning if ii is not used
    std::string ss = manager->getProperty("anotherprop");
    // Note that some versions of boost::lexical_cast > 1.34 give a string such
    // as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however
    // does
    // still give the correct 9.99.

    TS_ASSERT_EQUALS(ss.substr(0, 4), std::string("1.11"));

    // This works, but CANNOT at present declare the string on a separate line
    // and then assign
    //               (as I did for the int & double above)
    std::string sss = manager->getProperty("yetanotherprop");
    TS_ASSERT(!sss.compare("itsValue"));
  }

  void testGetProperties() {
    std::vector<Property *> props = manager->getProperties();
    TS_ASSERT(props.size() == 3);
    Property *p = props[0];
    TS_ASSERT(!p->name().compare("aProp"));
    TS_ASSERT(!p->value().compare("1"));
  }

  void testLongLongProperty() {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("llprop", static_cast<int64_t>(0)));
    TS_ASSERT_THROWS_NOTHING(mgr.setProperty("llprop", static_cast<int64_t>(52147900000)));
    TS_ASSERT_EQUALS(mgr.getPropertyValue("llprop"), "52147900000");
    TS_ASSERT_THROWS_NOTHING(mgr.setPropertyValue("llprop", "1234567890123456789"));
    int64_t retrieved(0);
    TS_ASSERT_THROWS_NOTHING(retrieved = mgr.getProperty("llprop"));
    TS_ASSERT_EQUALS(retrieved, static_cast<int64_t>(1234567890123456789));
  }

  void testRemoveProperty() {
    PropertyManagerHelper mgr;
    const std::string name("TestProperty");
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name, 10.0));
    TS_ASSERT_THROWS_NOTHING(mgr.removeProperty(name));
    TS_ASSERT_EQUALS(mgr.getProperties().size(), 0);
  }

  void testTakeProperty() {
    PropertyManagerHelper mgr;
    const std::string name("TestProperty");
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name, 10.0));
    std::unique_ptr<Property> propertyPointer;
    TS_ASSERT_THROWS_NOTHING(propertyPointer = mgr.takeProperty(0));
    TS_ASSERT(propertyPointer);
    TS_ASSERT_EQUALS(name, propertyPointer->name());
    TS_ASSERT_EQUALS(mgr.getProperties().size(), 0);
  }

  void testClear() {
    PropertyManagerHelper mgr;
    const std::string name("TestProperty");
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name + "1", 10.0));
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name + "2", 15.0));
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name + "3", 14.0));

    TS_ASSERT_EQUALS(mgr.propertyCount(), 3);
    TS_ASSERT_THROWS_NOTHING(mgr.clear());
    TS_ASSERT_EQUALS(mgr.propertyCount(), 0);
  }

  void testUpdatePropertyValues() {
    PropertyManagerHelper mgr1;
    mgr1.declareProperty("aProp", 10);
    PropertyManagerHelper mgr2;
    mgr2.declareProperty("aProp", 0);
    mgr2.updatePropertyValues(mgr1);
    const std::vector<Property *> &props1 = mgr1.getProperties();
    const std::vector<Property *> &props2 = mgr2.getProperties();
    TS_ASSERT_EQUALS(props1.size(), props2.size());
    TS_ASSERT_DIFFERS(&props1[0], &props2[0]);
    TS_ASSERT_EQUALS(props1[0]->name(), props2[0]->name());
    TS_ASSERT_EQUALS(props1[0]->value(), props2[0]->value());
  }

  void testUpdatePropertyValuesWithStringConversion() {
    PropertyManagerHelper mgr1;
    mgr1.declareProperty("aProp", "10");
    PropertyManagerHelper mgr2;
    mgr2.declareProperty("aProp", 0);
    mgr2.updatePropertyValues(mgr1);
    const std::vector<Property *> &props1 = mgr1.getProperties();
    const std::vector<Property *> &props2 = mgr2.getProperties();
    TS_ASSERT_EQUALS(props1.size(), props2.size());
    TS_ASSERT_DIFFERS(&props1[0], &props2[0]);
    TS_ASSERT_EQUALS(props1[0]->name(), props2[0]->name());
    TS_ASSERT_EQUALS(props1[0]->value(), props2[0]->value());
  }

  void test_asStringWithNotEnabledProperty() {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("Semaphor", true));
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("Crossing", 42));
    mgr.setPropertySettings(
        "Crossing", std::make_unique<EnabledWhenProperty>("Semaphor", Mantid::Kernel::ePropertyCriterion::IS_DEFAULT));

    TSM_ASSERT_EQUALS("Show the default", mgr.asString(true), "{\"Crossing\":42,\"Semaphor\":true}");
    mgr.setProperty("Semaphor", false);
    TSM_ASSERT_EQUALS("Hide not enabled", mgr.asString(true), "{\"Semaphor\":false}");
  }

  void test_asString() {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("Prop1", 10));
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty("Prop2", 15));

    ::Json::Value value;

    TSM_ASSERT("value was not valid JSON", Mantid::JsonHelpers::parse(mgr.asString(), &value));

    TSM_ASSERT_EQUALS("value was not empty", value.size(), 0);

    TSM_ASSERT_EQUALS("Show the default", mgr.asString(true), "{\"Prop1\":10,\"Prop2\":15}");

    TSM_ASSERT("value was not valid JSON", Mantid::JsonHelpers::parse(mgr.asString(true), &value));
    TS_ASSERT_EQUALS(boost::lexical_cast<int>(value["Prop1"].asString()), 10);
    TS_ASSERT_EQUALS(boost::lexical_cast<int>(value["Prop2"].asString()), 15);
    mgr.setProperty("Prop1", 123);
    mgr.setProperty("Prop2", 456);
    TSM_ASSERT_EQUALS("Change the values", mgr.asString(false), "{\"Prop1\":123,\"Prop2\":456}");

    TSM_ASSERT("value was not valid JSON", Mantid::JsonHelpers::parse(mgr.asString(false), &value));
    TS_ASSERT_EQUALS(boost::lexical_cast<int>(value["Prop1"].asString()), 123);
    TS_ASSERT_EQUALS(boost::lexical_cast<int>(value["Prop2"].asString()), 456);
  }

  void test_asStringWithArrayProperty() {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(std::make_unique<ArrayProperty<double>>("ArrayProp")));

    ::Json::Value value;

    TSM_ASSERT("value was not valid JSON", Mantid::JsonHelpers::parse(mgr.asString(), &value));

    TSM_ASSERT_EQUALS("value was not empty", value.size(), 0);

    TSM_ASSERT_EQUALS("Show the default", mgr.asString(true), "{\"ArrayProp\":[]}");

    TSM_ASSERT("value was not valid JSON", Mantid::JsonHelpers::parse(mgr.asString(true), &value));

    mgr.setProperty("ArrayProp", "10.1,12.5,23.5");

    TSM_ASSERT_EQUALS("Change the values", mgr.asString(false), "{\"ArrayProp\":[10.1,12.5,23.5]}");

    TSM_ASSERT("value was not valid JSON", Mantid::JsonHelpers::parse(mgr.asString(false), &value));
  }

  void test_asStringWithNonSerializableProperty() {
    using namespace Mantid::Kernel;
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(std::make_unique<MockNonSerializableProperty>("PropertyName", 0)));
    TS_ASSERT_EQUALS(mgr.asString(true), "null")
    TS_ASSERT_EQUALS(mgr.asString(false), "null")
    // Set to non-default value.
    mgr.setProperty("PropertyName", 1);
    TS_ASSERT_EQUALS(mgr.asString(true), "null")
    TS_ASSERT_EQUALS(mgr.asString(false), "null")
  }

  //-----------------------------------------------------------------------------------------------------------
  /** Test of adding managers together (this will be used when
   * concatenating runs together).
   */
  void testAdditionOperator() {
    PropertyManager mgr1;
    mgr1.declareProperty(std::make_unique<PropertyWithValue<double>>("double", 12.0), "docs");
    mgr1.declareProperty(std::make_unique<PropertyWithValue<int>>("int", 23), "docs");
    mgr1.declareProperty(std::make_unique<PropertyWithValue<double>>("double_only_in_mgr1", 456.0), "docs");

    PropertyManager mgr2;
    mgr2.declareProperty(std::make_unique<PropertyWithValue<double>>("double", 23.6), "docs");
    mgr2.declareProperty(std::make_unique<PropertyWithValue<int>>("int", 34), "docs");
    mgr2.declareProperty(std::make_unique<PropertyWithValue<double>>("new_double_in_mgr2", 321.0), "docs");
    mgr2.declareProperty(std::make_unique<PropertyWithValue<int>>("new_int", 655), "docs");

    // Add em together
    mgr1 += mgr2;

    double d;
    d = mgr1.getProperty("double");
    TS_ASSERT_DELTA(d, 35.6, 1e-4);
    d = mgr1.getProperty("double_only_in_mgr1");
    TS_ASSERT_DELTA(d, 456.0, 1e-4);
    d = mgr1.getProperty("new_double_in_mgr2");
    TS_ASSERT_DELTA(d, 321.0, 1e-4);

    int i;
    i = mgr1.getProperty("int");
    TS_ASSERT_EQUALS(i, 57);
    i = mgr1.getProperty("new_int");
    TS_ASSERT_EQUALS(i, 655);
  }

  void test_char_array() {
    PropertyManagerHelper mgr;

    auto nonEmptyString = std::make_shared<MandatoryValidator<std::string>>();
    mgr.declareProperty("SampleChemicalFormula", "", "Chemical composition of the sample material", nonEmptyString,
                        Direction::Input);

    TSM_ASSERT("Mandatory validator unsatisified.", !mgr.validateProperties());
    mgr.setProperty("SampleChemicalFormula", "CH3");
    TSM_ASSERT("Mandatory validator should be satisfied.", mgr.validateProperties());
  }

  void test_optional_bool_property() {
    PropertyManagerHelper mgr;

    mgr.declareProperty(
        std::make_unique<PropertyWithValue<OptionalBool>>(
            "PropertyX", OptionalBool::Unset, std::make_shared<MandatoryValidator<OptionalBool>>(), Direction::Input),
        "Custom property");

    TSM_ASSERT("Mandatory validator unsatisified.", !mgr.validateProperties());
    mgr.setProperty("PropertyX", OptionalBool(true));
    TSM_ASSERT("Mandatory validator should be satisfied.", mgr.validateProperties());
  }

  void test_setPropertiesWithSimpleString() {
    PropertyManagerHelper mgr;

    mgr.declareProperty(std::make_unique<PropertyWithValue<double>>("double", 12.0), "docs");
    mgr.declareProperty(std::make_unique<PropertyWithValue<int>>("int", 23), "docs");

    mgr.setPropertiesWithString("double= 13.0 ;int=22 ");
    double d = mgr.getProperty("double");
    int i = mgr.getProperty("int");
    TS_ASSERT_EQUALS(d, 13.0);
    TS_ASSERT_EQUALS(i, 22);

    mgr.setPropertiesWithString("double= 23.4 ;int=11", {"int"});
    d = mgr.getProperty("double");
    i = mgr.getProperty("int");
    TS_ASSERT_EQUALS(d, 23.4);
    TS_ASSERT_EQUALS(i, 22);

    mgr.setPropertiesWithString(R"({"double": 14.0, "int":33})");
    d = mgr.getProperty("double");
    i = mgr.getProperty("int");
    TS_ASSERT_EQUALS(d, 14.0);
    TS_ASSERT_EQUALS(i, 33);

    mgr.setPropertiesWithString(R"({"double": 12.3 ,"int":11})", {"int"});
    d = mgr.getProperty("double");
    i = mgr.getProperty("int");
    TS_ASSERT_EQUALS(d, 12.3);
    TS_ASSERT_EQUALS(i, 33);
  }

  void test_operator_equals_same_contents() {
    PropertyManager a;
    PropertyManager b;

    a.declareProperty("Prop1", 10);
    b.declareProperty("Prop1", 10);
    TS_ASSERT_EQUALS(a, b);
    TS_ASSERT(!(a != b));
  }

  void test_operator_not_equals_different_sizes() {
    PropertyManager a;
    PropertyManager b;

    b.declareProperty("Prop1", 10);
    TSM_ASSERT_DIFFERS("Unequal sizes", a, b);
    TSM_ASSERT("Unequal sizes", !(a == b));
  }

  void test_operator_not_equals_different_keys() {
    PropertyManager a;
    PropertyManager b;
    a.declareProperty("Prop1", 1);
    b.declareProperty("Prop2", 1);
    TSM_ASSERT_DIFFERS("Different Keys", a, b);
    TSM_ASSERT("Different Keys", !(a == b));
  }

  void test_operator_not_equals_different_vaues() {
    PropertyManager a;
    PropertyManager b;
    a.declareProperty("Prop1", 1);
    b.declareProperty("Prop1", 2);
    TSM_ASSERT_DIFFERS("Different Values", a, b);
    TSM_ASSERT("Different Values", !(a == b));
  }

private:
  std::unique_ptr<PropertyManagerHelper> manager;
};

//-------------------------------------------------------------------------------------------------
// Performance Test
//-------------------------------------------------------------------------------------------------

class PropertyManagerTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PropertyManagerTestPerformance *createSuite() { return new PropertyManagerTestPerformance(); }
  static void destroySuite(PropertyManagerTestPerformance *suite) { delete suite; }

  PropertyManagerTestPerformance() : m_manager(), m_filter(std::make_unique<LogFilter>(*createTestFilter())) {
    const size_t nprops = 2000;
    for (size_t i = 0; i < nprops; ++i) {
      m_manager.declareProperty(createTestSeries("prop" + boost::lexical_cast<std::string>(i)));
    }
  }

  void test_Perf_Of_Filtering_Large_Number_Of_Properties_By_Other_Property() {
    m_manager.filterByProperty(m_filter.get());
  }

private:
  /// Test manager
  PropertyManagerHelper m_manager;
  /// Test filter
  std::unique_ptr<LogFilter> m_filter;
};
