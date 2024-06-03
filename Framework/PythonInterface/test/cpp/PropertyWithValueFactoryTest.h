// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4005)
#endif
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#ifdef _MSC_VER
#pragma warning(push)
#endif

#include "MantidKernel/PropertyWithValue.h"
#include <boost/python/extract.hpp>

//-------------------------------------------------------------------------

using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::Converters::PySequenceToVector;
using Mantid::PythonInterface::Registry::PropertyWithValueFactory;

class PropertyWithValueFactoryTest : public CxxTest::TestSuite {
public:
  static PropertyWithValueFactoryTest *createSuite() { return new PropertyWithValueFactoryTest(); }
  static void destroySuite(PropertyWithValueFactoryTest *suite) { delete suite; }

#define FROM_INT PyLong_FromLong
#define FROM_CSTRING PyUnicode_FromString

#define CREATE_PROPERTY_TEST_BODY(CType, PythonCall)                                                                   \
                                                                                                                       \
  {                                                                                                                    \
    using namespace boost::python;                                                                                     \
    using namespace Mantid::Kernel;                                                                                    \
    object pyvalue = object(handle<>(PythonCall));                                                                     \
    auto valueProp = createAndCheckPropertyTraits<CType>("TestProperty", pyvalue, Direction::Input);                   \
    checkPropertyValue<CType>(std::move(valueProp), pyvalue);                                                          \
  }

  void test_builtin_type_creates_int_type_property_without_error() { testCreateSingleValueProperty<int>(FROM_INT(10)); }

  void test_builtin_type_creates_double_type_property_without_error() {
    testCreateSingleValueProperty<double>(PyFloat_FromDouble(50.123));
  }

  void test_builtin_type_creates_string_type_property_without_error() {
    testCreateSingleValueProperty<std::string>(FROM_CSTRING("unit"));
  }

  void test_builtin_type_create_double_array_from_tuple_type_property() {
    testCreateArrayProperty<double>(Py_BuildValue("(ff)", 0.5, 1.45));
  }

  void test_builtin_type_create_string_array_from_tuple_type_property() {
    testCreateArrayProperty<std::string>(Py_BuildValue("(ss)", "Test1", "Pass2"));
  }

  void test_builtin_type_create_long_array_from_list_type_property() {
    testCreateArrayProperty<int>(Py_BuildValue("[NN]", PyLong_FromLong(-10), PyLong_FromLong(4)));
  }

  void test_builtin_type_create_int_array_from_list_type_property() {
    testCreateArrayProperty<int>(Py_BuildValue("[ii]", -10, 4));
  }

private:
  template <typename CType> void testCreateSingleValueProperty(PyObject *pyValue) {
    using namespace boost::python;
    using namespace Mantid::Kernel;
    object pyvalue = object(handle<>(pyValue));
    auto valueProp = createAndCheckPropertyTraits<CType>("TestProperty", pyvalue, Direction::Input);
    checkPropertyValue<CType>(std::move(valueProp), pyvalue);
  }

  template <typename CType> void testCreateArrayProperty(PyObject *pyValue) {
    using namespace boost::python;
    using namespace Mantid::Kernel;
    using TypeVec = std::vector<CType>;
    object pyvalue = object(handle<>(pyValue));
    auto valueProp = createAndCheckPropertyTraits<TypeVec>("TestProperty", pyvalue, Direction::Input);
    checkArrayPropertyValue<CType>(std::move(valueProp), pyvalue);
  }

  template <typename ExpectedType>
  std::unique_ptr<PropertyWithValue<ExpectedType>> createAndCheckPropertyTraits(const std::string &name,
                                                                                const boost::python::object &value,
                                                                                const unsigned int direction) {
    using Mantid::Kernel::Property;
    std::unique_ptr<Property> namedProp;
    TS_ASSERT_THROWS_NOTHING(namedProp = PropertyWithValueFactory::create(name, value, direction));
    TS_ASSERT(namedProp);
    // Is it correctly typed
    std::unique_ptr<PropertyWithValue<ExpectedType>> typedProp(
        dynamic_cast<PropertyWithValue<ExpectedType> *>(namedProp.release()));
    TS_ASSERT(typedProp);

    // Traits
    TS_ASSERT_EQUALS(typedProp->name(), name);
    TS_ASSERT_EQUALS(typedProp->direction(), direction);
    return typedProp;
  }

  template <typename ValueType>
  void checkPropertyValue(std::unique_ptr<PropertyWithValue<ValueType>> valueProp,
                          const boost::python::object &expectedValue) {
    const ValueType srcValue = boost::python::extract<ValueType>(expectedValue);
    const ValueType propValue = (*valueProp)();
    TS_ASSERT_EQUALS(srcValue, propValue);
  }

  template <typename ValueType>
  void checkArrayPropertyValue(std::shared_ptr<PropertyWithValue<std::vector<ValueType>>> valueProp,
                               const boost::python::object &expectedValue) {
    const auto srcValue = PySequenceToVector<ValueType>(expectedValue)();
    const auto propValue = (*valueProp)();
    // Check size
    TS_ASSERT_EQUALS(srcValue.size(), propValue.size());
    // Check first element
    TS_ASSERT_EQUALS(srcValue[0], propValue[0]);
  }
};
