//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/SequenceTypeHandler.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/python/extract.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
namespace {
template <typename HeldType> struct StdVectorExtractor {
  static std::vector<HeldType> extract(const boost::python::object &value) {
    return boost::python::extract<std::vector<HeldType>>(value.ptr());
  }
};
template <> struct StdVectorExtractor<bool> {
  static std::vector<bool> extract(const boost::python::object &) {
    throw std::runtime_error(
        "Unable to supported extracting std::vector<bool> from python object");
  }
};
} // namespace

/**
 * Set function to handle Python -> C++ calls to a property manager and get the
 * correct type
 * @param alg :: A pointer to an IPropertyManager
 * @param name :: The name of the property
 * @param value :: A boost python object that stores the container values
 */
template <typename ContainerType>
void SequenceTypeHandler<ContainerType>::set(
    Kernel::IPropertyManager *alg, const std::string &name,
    const boost::python::object &value) const {
  using namespace boost::python;
  using DestElementType = typename ContainerType::value_type;

  // Current workaround for things that still pass back wrapped vectors...
  if (boost::starts_with(value.ptr()->ob_type->tp_name, "std_vector")) {
    alg->setProperty(name, StdVectorExtractor<DestElementType>::extract(value));
  }
  // numpy arrays requires special handling to extract their types. Hand-off to
  // a more appropriate handler
  else if (NumPy::NdArray::check(value)) {
    alg->setProperty(name, Converters::NDArrayToVector<DestElementType>(
                               NumPy::NdArray(value))());
  } else if (PySequence_Check(value.ptr())) {
    alg->setProperty(name,
                     Converters::PySequenceToVector<DestElementType>(value)());
  } else // assume it is a scalar and try to convert into a vector of length one
  {
    DestElementType scalar =
        boost::python::extract<DestElementType>(value.ptr());
    alg->setProperty(name, std::vector<DestElementType>(1, scalar));
  }
}

/**
 * Create a PropertyWithValue from the given python object value
 * @param name :: The name of the property
 * @param defaultValue :: The defaultValue of the property. The object attempts
 * to extract
 * a value of type ContainerType from the python object
 * @param validator :: A python object pointing to a validator instance, which
 * can be None.
 * @param direction :: The direction of the property
 * @returns A pointer to a newly constructed property instance
 */
template <typename ContainerType>
std::unique_ptr<Kernel::Property> SequenceTypeHandler<ContainerType>::create(
    const std::string &name, const boost::python::object &defaultValue,
    const boost::python::object &validator,
    const unsigned int direction) const {
  using DestElementType = typename ContainerType::value_type;
  using Kernel::IValidator;
  using Kernel::PropertyWithValue;
  using boost::python::extract;

  ContainerType valueInC;
  // Current workaround for things that still pass back wrapped vectors...
  if (boost::starts_with(defaultValue.ptr()->ob_type->tp_name, "std_vector")) {
    valueInC = StdVectorExtractor<DestElementType>::extract(defaultValue);
  } else if (PySequence_Check(defaultValue.ptr())) {
    valueInC = Converters::PySequenceToVector<DestElementType>(defaultValue)();
  } else // assume it is a scalar and try to convert into a vector of length one
  {
    DestElementType scalar =
        boost::python::extract<DestElementType>(defaultValue.ptr());
    valueInC = std::vector<DestElementType>(1, scalar);
  }

  std::unique_ptr<Kernel::Property> valueProp;
  if (isNone(validator)) {
    valueProp = Mantid::Kernel::make_unique<PropertyWithValue<ContainerType>>(
        name, valueInC, direction);
  } else {
    const IValidator *propValidator = extract<IValidator *>(validator);
    valueProp = Mantid::Kernel::make_unique<PropertyWithValue<ContainerType>>(
        name, valueInC, propValidator->clone(), direction);
  }
  return valueProp;
}

//-----------------------------------------------------------------------
// Concrete instantiations
//-----------------------------------------------------------------------
///@cond
#define INSTANTIATE(ElementType)                                               \
  template struct DLLExport SequenceTypeHandler<std::vector<ElementType>>;

INSTANTIATE(int)
INSTANTIATE(long)
INSTANTIATE(long long)
INSTANTIATE(unsigned int)
INSTANTIATE(unsigned long)
INSTANTIATE(unsigned long long)
INSTANTIATE(double)
INSTANTIATE(std::string)
INSTANTIATE(bool)
///@endcond
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid
