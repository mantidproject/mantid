#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/NullValidator.h"

#include "MantidPythonInterface/kernel/Converters/ContainerDtype.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/kernel/NdArray.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>

#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::IValidator_sptr;
using Mantid::Kernel::NullValidator;
using Mantid::Kernel::PropertyWithValue;
namespace Converters = Mantid::PythonInterface::Converters;
namespace NumPy = Mantid::PythonInterface::NumPy;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

namespace {
/// return_value_policy for cloned numpy array
using return_cloned_numpy =
    return_value_policy<Policies::VectorRefToNumpy<Converters::Clone>>;

// Call the dtype helper function
template <typename type> std::string dtype(ArrayProperty<type> &self) {
  return Converters::dtype(self);
}

#define EXPORT_ARRAY_PROP(type, prefix)                                        \
  class_<ArrayProperty<type>, bases<PropertyWithValue<std::vector<type>>>,     \
         boost::noncopyable>(#prefix "ArrayProperty", no_init)                 \
      .def(init<const std::string &, const unsigned int>(                      \
          (arg("self"), arg("name"), arg("direction") = Direction::Input),     \
          "Construct an ArrayProperty of type " #type))                        \
                                                                               \
      .def(init<const std::string &, IValidator_sptr, const unsigned int>(     \
          (arg("self"), arg("name"), arg("validator"),                         \
           arg("direction") = Direction::Input),                               \
          "Construct an ArrayProperty of type " #type " with a validator"))    \
                                                                               \
      .def(init<const std::string &, const std::string &, IValidator_sptr,     \
                const unsigned int>(                                           \
          (arg("self"), arg("name"), arg("values"),                            \
           arg("validator") = IValidator_sptr(new NullValidator),              \
           arg("direction") = Direction::Input),                               \
          "Construct an ArrayProperty of type " #type                          \
          " with a validator giving the values as a string"))                  \
      .def("__init__",                                                         \
           make_constructor(                                                   \
               &createArrayPropertyFromList<type>, default_call_policies(),    \
               (arg("name"), arg("values"),                                    \
                arg("validator") = IValidator_sptr(new NullValidator),         \
                arg("direction") = Direction::Input)))                         \
      .def("__init__",                                                         \
           make_constructor(                                                   \
               &createArrayPropertyFromNDArray<type>, default_call_policies(), \
               (arg("name"), arg("values"),                                    \
                arg("validator") = IValidator_sptr(new NullValidator),         \
                arg("direction") = Direction::Input)))                         \
      .def("dtype", &dtype<type>, arg("self"))                                 \
      .add_property("value", make_function(&ArrayProperty<type>::operator(),   \
                                           return_cloned_numpy()));

/**
 * Factory function to allow the initial values to be specified as a python list
 * @param name :: The name of the property
 * @param vec :: A boost python list of initial values
 * @param validator :: A validator object
 * @param direction :: A direction
 * @return
 */
template <typename T>
ArrayProperty<T> *createArrayPropertyFromList(const std::string &name,
                                              const boost::python::list &values,
                                              IValidator_sptr validator,
                                              const unsigned int direction) {
  return new ArrayProperty<T>(name, Converters::PySequenceToVector<T>(values)(),
                              validator, direction);
}

/**
 * Factory function to allow the initial values to be specified as a numpy
 * array
 * @param name :: The name of the property
 * @param vec :: A boost python array of initial values
 * @param validator :: A validator object
 * @param direction :: A direction
 * @return
 */
template <typename T>
ArrayProperty<T> *createArrayPropertyFromNDArray(const std::string &name,
                                                 const NumPy::NdArray &values,
                                                 IValidator_sptr validator,
                                                 const unsigned int direction) {
  return new ArrayProperty<T>(name, Converters::NDArrayToVector<T>(values)(),
                              validator, direction);
}
} // namespace

void export_ArrayProperty() {
  // Match the python names to their C types
  EXPORT_ARRAY_PROP(double, Float);
  EXPORT_ARRAY_PROP(long, Int);
  EXPORT_ARRAY_PROP(std::string, String);

  // Needs these declarations also to ensure that properties not created in
  // Python can be seen also. Users shouldn't need this
  EXPORT_ARRAY_PROP(int, CInt);
  EXPORT_ARRAY_PROP(size_t, UnsignedInt);
}