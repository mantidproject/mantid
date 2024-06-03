// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/Registry/MappingTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <memory>

#include <cassert>

using Mantid::Kernel::TimeSeriesProperty;
using Mantid::PythonInterface::Registry::PropertyWithValueFactory;
using namespace boost::python;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;

namespace Mantid::PythonInterface::Registry {
namespace {
/// Lookup map type
using PyTypeIndex = std::map<const PyTypeObject *, std::shared_ptr<PropertyValueHandler>>;

/**
 * Initialize lookup map
 */
void initTypeLookup(PyTypeIndex &index) {
  assert(index.empty());

  // Map the Python types to the best match in C++
  using FloatHandler = TypedPropertyValueHandler<double>;
  index.emplace(&PyFloat_Type, std::make_shared<FloatHandler>());

  using BoolHandler = TypedPropertyValueHandler<bool>;
  index.emplace(&PyBool_Type, std::make_shared<BoolHandler>());

  using IntHandler = TypedPropertyValueHandler<int>;
  index.emplace(&PyLong_Type, std::make_shared<IntHandler>());

  // In Python 3 all strings are unicode but in Python 2 unicode strings
  // must be explicitly requested. The C++ string handler will accept both
  // but throw and error if the unicode string contains non-ascii characters
  using AsciiStrHandler = TypedPropertyValueHandler<std::string>;
  // Both versions have unicode objects
  index.emplace(&PyUnicode_Type, std::make_shared<AsciiStrHandler>());
  // Handle a dictionary type
  index.emplace(&PyDict_Type, std::make_shared<MappingTypeHandler>());
}

/**
 * Returns a reference to the static lookup map
 */
const PyTypeIndex &getTypeIndex() {
  static PyTypeIndex index;
  if (index.empty())
    initTypeLookup(index);
  return index;
}

// Lookup map for arrays
using PyArrayIndex = std::map<std::string, std::shared_ptr<PropertyValueHandler>>;

/**
 * Initialize lookup map
 */
void initArrayLookup(PyArrayIndex &index) {
  assert(index.empty());

  // Map the Python array types to the best match in C++
  using FloatArrayHandler = SequenceTypeHandler<std::vector<double>>;
  index.emplace("FloatArray", std::make_shared<FloatArrayHandler>());

  using StringArrayHandler = SequenceTypeHandler<std::vector<std::string>>;
  index.emplace("StringArray", std::make_shared<StringArrayHandler>());

  using IntArrayHandler = SequenceTypeHandler<std::vector<int>>;
  index.emplace("IntArray", std::make_shared<IntArrayHandler>());
}

/**
 * Returns a reference to the static array lookup map
 */
const PyArrayIndex &getArrayIndex() {
  static PyArrayIndex index;
  if (index.empty())
    initArrayLookup(index);
  return index;
}
} // namespace

/**
 * Creates a PropertyWithValue<Type> instance from the given information.
 * The python type is mapped to a C type using the mapping defined by
 * initPythonTypeMap()
 * @param name :: The name of the property
 * @param defaultValue :: A default value for this property.
 * @param validator :: A validator object
 * @param direction :: Specifies whether the property is Input, InOut or Output
 * @returns A pointer to a new Property object
 */
std::unique_ptr<Kernel::Property> PropertyWithValueFactory::create(const std::string &name,
                                                                   const boost::python::object &defaultValue,
                                                                   const boost::python::object &validator,
                                                                   const unsigned int direction) {
  const auto &propHandle = lookup(defaultValue.ptr());
  return propHandle.create(name, defaultValue, validator, direction);
}

/**
 * Creates a PropertyWithValue<Type> instance from the given information.
 * The python type is mapped to a C type using the mapping defined by
 * initPythonTypeMap()
 * @param name :: The name of the property
 * @param defaultValue :: A default value for this property.
 * @param direction :: Specifies whether the property is Input, InOut or Output
 * @returns A pointer to a new Property object
 */
std::unique_ptr<Kernel::Property> PropertyWithValueFactory::create(const std::string &name,
                                                                   const boost::python::object &defaultValue,
                                                                   const unsigned int direction) {
  boost::python::object validator; // Default construction gives None object
  return create(name, defaultValue, validator, direction);
}

/**
 * Creates a TimeSeriesProperty<Type> instance from the given information.
 * The python type is mapped to a C type
 * @param name :: The name of the property
 * @param defaultValue :: A default value for this property.
 * @returns A pointer to a new Property object
 */
std::unique_ptr<Mantid::Kernel::Property> PropertyWithValueFactory::createTimeSeries(const std::string &name,
                                                                                     const list &defaultValue) {

  // Use a PyObject pointer to determine the type stored in the list
  auto obj = object(defaultValue[0]).ptr();
  auto val = defaultValue[0];

  /**
   * Decide which kind of TimeSeriesProperty to return
   * Need to use a different method to check for boolean values
   * since extract<> seems to get confused sometimes.
   */
  if (PyBool_Check(obj)) {
    return std::make_unique<TimeSeriesProperty<bool>>(name);
  } else if (extract<int>(val).check()) {
    return std::make_unique<TimeSeriesProperty<int>>(name);
  } else if (extract<double>(val).check()) {
    return std::make_unique<TimeSeriesProperty<double>>(name);
  } else if (extract<std::string>(val).check()) {
    return std::make_unique<TimeSeriesProperty<std::string>>(name);
  }

  // If we reach here an error has occurred as there are no type to create
  // a TimeSeriesProperty from
  throw std::runtime_error("Cannot create a TimeSeriesProperty with that data type!");
}

//-------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------
/**
 * Return a handler that maps the python type to a C++ type
 * @param object :: A pointer to a PyObject that represents the type
 * @returns A pointer to handler that can be used to instantiate a property
 */
const PropertyValueHandler &PropertyWithValueFactory::lookup(PyObject *const object) {
  // Check if object is array.
  const auto arrayType = isArray(object);
  if (!arrayType.empty()) {
    const PyArrayIndex &arrayIndex = getArrayIndex();
    auto ait = arrayIndex.find(arrayType);
    if (ait != arrayIndex.end()) {
      return *(ait->second);
    }
  }
  // Object is not array, so check primitive types
  const PyTypeIndex &typeIndex = getTypeIndex();
  auto cit = typeIndex.find(object->ob_type);
  if (cit == typeIndex.end()) {
    std::ostringstream os;
    os << "Cannot create PropertyWithValue from Python type " << object->ob_type->tp_name
       << ". No converter registered in PropertyWithValueFactory.";
    throw std::invalid_argument(os.str());
  }
  return *(cit->second);
}

/**
 * Return a string for the array type to check the map for.
 * @param object :: Python object to check if it's an array
 * @return :: A string as the array type.
 */
const std::string PropertyWithValueFactory::isArray(PyObject *const object) {
  if (PyList_Check(object) || PyTuple_Check(object)) {
    // If we are dealing with an empty list/tuple, then we cannot deduce the
    // ArrayType. We need to throw at this point.
    if (PySequence_Size(object) < 1) {
      throw std::runtime_error("Cannot have a sequence type of length zero in a mapping type.");
    }

    PyObject *item = PySequence_Fast_GET_ITEM(object, 0);
    // Boolean can be cast to int, so check first.
    GNU_DIAG_OFF("parentheses-equality")
    if (PyBool_Check(item)) {
      throw std::runtime_error("Unable to support extracting arrays of booleans.");
    }
    if (PyLong_Check(item)) {
      return std::string("IntArray");
    }
    GNU_DIAG_ON("parentheses-equality")
    if (PyFloat_Check(item)) {
      return std::string("FloatArray");
    }
    if (PyUnicode_Check(item)) {
      return std::string("StringArray");
    }
    if (PyBytes_Check(item)) {
      return std::string("StringArray");
    }
    // If we get here, we've found a sequence and we can't interpret the item
    // type.
    std::ostringstream os;
    os << "Cannot create PropertyWithValue from Python type " << object->ob_type->tp_name
       << " containing items of type " << item->ob_type << ". No converter registered in PropertyWithValueFactory.";

    throw std::invalid_argument(os.str());
  } else {
    return std::string("");
  }
}
} // namespace Mantid::PythonInterface::Registry
