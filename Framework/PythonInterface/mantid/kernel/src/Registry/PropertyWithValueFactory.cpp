//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#include "MantidPythonInterface/kernel/Registry/MappingTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/make_shared.hpp>

#include <cassert>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
namespace {
/// Lookup map type
typedef std::map<PyTypeObject const *, boost::shared_ptr<PropertyValueHandler>>
    PyTypeIndex;

/**
 * Initialize lookup map
 */
void initTypeLookup(PyTypeIndex &index) {
  assert(index.empty());

  // Map the Python types to the best match in C++
  typedef TypedPropertyValueHandler<double> FloatHandler;
  index.emplace(&PyFloat_Type, boost::make_shared<FloatHandler>());

  typedef TypedPropertyValueHandler<bool> BoolHandler;
  index.emplace(&PyBool_Type, boost::make_shared<BoolHandler>());

  // Python 2/3 have an arbitrary-sized long type. The handler
  // will raise an error if the input value overflows a C long
  typedef TypedPropertyValueHandler<long> IntHandler;
  index.emplace(&PyLong_Type, boost::make_shared<IntHandler>());

  // In Python 3 all strings are unicode but in Python 2 unicode strings
  // must be explicitly requested. The C++ string handler will accept both
  // but throw and error if the unicode string contains non-ascii characters
  typedef TypedPropertyValueHandler<std::string> AsciiStrHandler;
  // Both versions have unicode objects
  index.emplace(&PyUnicode_Type, boost::make_shared<AsciiStrHandler>());

#if PY_MAJOR_VERSION < 3
  // Version < 3 had separate fixed-precision long and arbitrary precision
  // long objects. Handle these too
  index.emplace(&PyInt_Type, boost::make_shared<IntHandler>());
  // Version 2 also has the PyString_Type
  index.emplace(&PyString_Type, boost::make_shared<AsciiStrHandler>());
#endif

  // Handle a dictionary type
  index.emplace(&PyDict_Type, boost::make_shared<MappingTypeHandler>());
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
typedef std::map<std::string, boost::shared_ptr<PropertyValueHandler>>
    PyArrayIndex;

/**
 * Initialize lookup map
 */
void initArrayLookup(PyArrayIndex &index) {
  assert(index.empty());

  // Map the Python array types to the best match in C++
  typedef SequenceTypeHandler<std::vector<double>> FloatArrayHandler;
  index.emplace("FloatArray", boost::make_shared<FloatArrayHandler>());

  typedef SequenceTypeHandler<std::vector<std::string>> StringArrayHandler;
  index.emplace("StringArray", boost::make_shared<StringArrayHandler>());

  typedef SequenceTypeHandler<std::vector<long>> LongIntArrayHandler;
  index.emplace("LongIntArray", boost::make_shared<LongIntArrayHandler>());

#if PY_MAJOR_VERSION < 3
  // Backwards compatible behaviour
  typedef SequenceTypeHandler<std::vector<int>> IntArrayHandler;
  index.emplace("IntArray", boost::make_shared<IntArrayHandler>());
#endif
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
}

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
std::unique_ptr<Kernel::Property> PropertyWithValueFactory::create(
    const std::string &name, const boost::python::object &defaultValue,
    const boost::python::object &validator, const unsigned int direction) {
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
std::unique_ptr<Kernel::Property>
PropertyWithValueFactory::create(const std::string &name,
                                 const boost::python::object &defaultValue,
                                 const unsigned int direction) {
  boost::python::object validator; // Default construction gives None object
  return create(name, defaultValue, validator, direction);
}

//-------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------
/**
 * Return a handler that maps the python type to a C++ type
 * @param object :: A pointer to a PyObject that represents the type
 * @returns A pointer to handler that can be used to instantiate a property
 */
const PropertyValueHandler &
PropertyWithValueFactory::lookup(PyObject *const object) {
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
    os << "Cannot create PropertyWithValue from Python type "
       << object->ob_type->tp_name
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
      throw std::runtime_error(
          "Cannot have a sequence type of length zero in a mapping type.");
    }

    PyObject *item = PySequence_Fast_GET_ITEM(object, 0);
    // Boolean can be cast to int, so check first.
    if (PyBool_Check(item)) {
      throw std::runtime_error(
          "Unable to support extracting arrays of booleans.");
    }
    if (PyLong_Check(item)) {
      return std::string("LongIntArray");
    }
#if PY_MAJOR_VERSION < 3
    // In python 2 ints & longs are separate
    if (PyInt_Check(item)) {
      return std::string("IntArray");
    }
#endif
    if (PyFloat_Check(item)) {
      return std::string("FloatArray");
    }
#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(item)) {
      return std::string("StringArray");
    }
#endif
    if (PyBytes_Check(item)) {
      return std::string("StringArray");
    }
    // If we get here, we've found a sequence and we can't interpret the item
    // type.
    std::ostringstream os;
    os << "Cannot create PropertyWithValue from Python type "
       << object->ob_type->tp_name << " containing items of type "
       << item->ob_type
       << ". No converter registered in PropertyWithValueFactory.";

    throw std::invalid_argument(os.str());
  } else {
    return std::string("");
  }
}
}
}
}
