// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/NullValidator.h"

#include "MantidPythonInterface/core/Converters/ContainerDtype.h"
#include "MantidPythonInterface/core/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>

#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::IValidator_sptr;
using Mantid::Kernel::NullValidator;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::NDArray;
namespace Converters = Mantid::PythonInterface::Converters;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

namespace {
/// return_value_policy for cloned numpy array
using return_cloned_numpy = return_value_policy<Policies::VectorRefToNumpy<Converters::Clone>>;

// Call the dtype helper function
template <typename type> std::string dtype(ArrayProperty<type> &self) { return Converters::dtype(self); }

// Check for the special case of a string
template <> std::string dtype(ArrayProperty<std::string> &self) {
  // Get a vector of all the strings
  std::vector<std::string> values = self();

  // Vector of ints to store the sizes of each of the strings
  std::vector<size_t> stringSizes;

  // Block allocate memory
  stringSizes.reserve(self.size());

  // Loop for the number of strings
  // For each string store the number of characters
  for (auto const &val : values) {
    auto size = val.size();
    stringSizes.emplace_back(size);
  }

  // Find the maximum number of characters
  size_t max = *std::max_element(std::begin(stringSizes), std::end(stringSizes));

  // Create the string to return
  std::stringstream ss;
  ss << "S" << max;
  std::string retVal = ss.str();
  return retVal;
}

#define EXPORT_ARRAY_PROP(type, prefix)                                                                                \
  class_<ArrayProperty<type>, bases<PropertyWithValue<std::vector<type>>>, boost::noncopyable>(                        \
      #prefix "ArrayProperty", no_init)                                                                                \
      .def(init<const std::string &, const unsigned int>(                                                              \
          (arg("self"), arg("name"), arg("direction") = Direction::Input),                                             \
          "Construct an ArrayProperty of type " #type))                                                                \
                                                                                                                       \
      .def(init<const std::string &, IValidator_sptr, const unsigned int>(                                             \
          (arg("self"), arg("name"), arg("validator"), arg("direction") = Direction::Input),                           \
          "Construct an ArrayProperty of type " #type " with a validator"))                                            \
                                                                                                                       \
      .def(init<const std::string &, const std::string &, IValidator_sptr, const unsigned int>(                        \
          (arg("self"), arg("name"), arg("values"), arg("validator") = IValidator_sptr(new NullValidator),             \
           arg("direction") = Direction::Input),                                                                       \
          "Construct an ArrayProperty of type " #type " with a validator giving the values as a string"))              \
      .def("__init__",                                                                                                 \
           make_constructor(&createArrayPropertyFromList<type>, default_call_policies(),                               \
                            (arg("name"), arg("values"), arg("validator") = IValidator_sptr(new NullValidator),        \
                             arg("direction") = Direction::Input)))                                                    \
      .def("__init__",                                                                                                 \
           make_constructor(&createArrayPropertyFromNDArray<type>, default_call_policies(),                            \
                            (arg("name"), arg("values"), arg("validator") = IValidator_sptr(new NullValidator),        \
                             arg("direction") = Direction::Input)))                                                    \
      .def("dtype", &dtype<type>, arg("self"))                                                                         \
      .add_property("value", make_function(&ArrayProperty<type>::operator(), return_cloned_numpy()));

/**
 * Factory function to allow the initial values to be specified as a python list
 * @param name :: The name of the property
 * @param vec :: A boost python list of initial values
 * @param validator :: A validator object
 * @param direction :: A direction
 * @return
 */
template <typename T>
ArrayProperty<T> *createArrayPropertyFromList(const std::string &name, const boost::python::list &values,
                                              const IValidator_sptr &validator, const unsigned int direction) {
  return new ArrayProperty<T>(name, Converters::PySequenceToVector<T>(values)(), validator, direction);
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
ArrayProperty<T> *createArrayPropertyFromNDArray(const std::string &name, const NDArray &values,
                                                 const IValidator_sptr &validator, const unsigned int direction) {
  return new ArrayProperty<T>(name, Converters::NDArrayToVector<T>(values)(), validator, direction);
}
} // namespace

void export_ArrayProperty() {
  // Match the python names to their C types
  EXPORT_ARRAY_PROP(double, Float);
  EXPORT_ARRAY_PROP(int, Int);
  EXPORT_ARRAY_PROP(std::string, String);

  // Needs these declarations also to ensure that properties not created in
  // Python can be seen also. Users shouldn't need this
  EXPORT_ARRAY_PROP(int, CInt);
  EXPORT_ARRAY_PROP(size_t, UnsignedInt);
}
