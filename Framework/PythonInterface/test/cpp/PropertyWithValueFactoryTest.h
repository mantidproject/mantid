#ifndef PROPERTYWITHVALUEFACTORYTEST_H_
#define PROPERTYWITHVALUEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4005)
#endif
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
#ifdef _MSC_VER
#pragma warning(push)
#endif

#include "MantidKernel/PropertyWithValue.h"
#include <boost/python/extract.hpp>

//-------------------------------------------------------------------------

using Mantid::PythonInterface::Registry::PropertyWithValueFactory;
using Mantid::PythonInterface::Converters::PySequenceToVector;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::Direction;

class PropertyWithValueFactoryTest : public CxxTest::TestSuite {
public:
#define CREATE_PROPERTY_TEST_BODY(CType, PythonCall)                           \
                                                                               \
  {                                                                            \
    using namespace boost::python;                                             \
    using namespace Mantid::Kernel;                                            \
    object pyvalue = object(handle<>(PythonCall));                             \
    boost::shared_ptr<PropertyWithValue<CType>> valueProp =                    \
        createAndCheckPropertyTraits<CType>("TestProperty", pyvalue,           \
                                            Direction::Input);                 \
    checkPropertyValue<CType>(valueProp, pyvalue);                             \
  }

#define CREATE_ARRAY_PROPERTY_TEST_BODY(CType, PythonCall)                     \
                                                                               \
  {                                                                            \
    using namespace boost::python;                                             \
    using namespace Mantid::Kernel;                                            \
    typedef std::vector<CType> TypeVec;                                        \
    object pyvalue = object(handle<>(PythonCall));                             \
    boost::shared_ptr<PropertyWithValue<TypeVec>> valueProp =                  \
        createAndCheckPropertyTraits<TypeVec>("TestProperty", pyvalue,         \
                                              Direction::Input);               \
    checkArrayPropertyValue<CType>(valueProp, pyvalue);                        \
  }

  void test_builtin_type_creates_int_type_property_without_error() {
    CREATE_PROPERTY_TEST_BODY(long, PyInt_FromLong(10));
  }

  void test_builtin_type_creates_double_type_property_without_error() {
    CREATE_PROPERTY_TEST_BODY(double, PyFloat_FromDouble(50.123));
  }

  void test_builtin_type_creates_string_type_property_without_error() {
    CREATE_PROPERTY_TEST_BODY(std::string, PyString_FromString("unit"));
  }

  void test_builtin_type_create_double_array_from_tuple_type_property() {
    CREATE_ARRAY_PROPERTY_TEST_BODY(double, Py_BuildValue("(ff)", 0.5, 1.45));
  }

  void test_builtin_type_create_string_array_from_tuple_type_property() {
    CREATE_ARRAY_PROPERTY_TEST_BODY(std::string,
                                    Py_BuildValue("(ss)", "Test1", "Pass2"));
  }

  void test_builtin_type_create_long_array_from_list_type_property() {
    CREATE_ARRAY_PROPERTY_TEST_BODY(
        long, Py_BuildValue("[NN]", PyLong_FromLong(-10), PyLong_FromLong(4)));
  }

  void test_builtin_type_create_int_array_from_list_type_property() {
    CREATE_ARRAY_PROPERTY_TEST_BODY(int, Py_BuildValue("[ii]", -10, 4));
  }

private:
  template <typename ExpectedType>
  boost::shared_ptr<PropertyWithValue<ExpectedType>>
  createAndCheckPropertyTraits(const std::string &name,
                               const boost::python::object &value,
                               const unsigned int direction) {
    using Mantid::Kernel::Property;
    Property *namedProp(NULL);
    TS_ASSERT_THROWS_NOTHING(
        namedProp = PropertyWithValueFactory::create(name, value, direction));
    TS_ASSERT(namedProp);
    // Is it correctly typed
    PropertyWithValue<ExpectedType> *typedProp =
        dynamic_cast<PropertyWithValue<ExpectedType> *>(namedProp);
    TS_ASSERT(typedProp);

    // Traits
    TS_ASSERT_EQUALS(namedProp->name(), name);
    TS_ASSERT_EQUALS(namedProp->direction(), direction);
    return boost::shared_ptr<PropertyWithValue<ExpectedType>>(typedProp);
  }

  template <typename ValueType>
  void
  checkPropertyValue(boost::shared_ptr<PropertyWithValue<ValueType>> valueProp,
                     const boost::python::object &expectedValue) {
    const ValueType srcValue = boost::python::extract<ValueType>(expectedValue);
    const ValueType propValue = (*valueProp)();
    TS_ASSERT_EQUALS(srcValue, propValue);
  }

  template <typename ValueType>
  void checkArrayPropertyValue(
      boost::shared_ptr<PropertyWithValue<std::vector<ValueType>>> valueProp,
      const boost::python::object &expectedValue) {
    const auto srcValue = PySequenceToVector<ValueType>(expectedValue)();
    const auto propValue = (*valueProp)();
    // Check size
    TS_ASSERT_EQUALS(srcValue.size(), propValue.size());
    // Check first element
    TS_ASSERT_EQUALS(srcValue[0], propValue[0]);
  }
};

#endif /* PROPERTYWITHVALUEFACTORYTEST_H_ */
