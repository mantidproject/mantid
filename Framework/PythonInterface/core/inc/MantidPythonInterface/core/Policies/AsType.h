// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_ASTYPE_H_
#define MANTID_PYTHONINTERFACE_ASTYPE_H_

#include <boost/mpl/and.hpp>
#include <boost/python/detail/prefix.hpp>
#include <boost/python/to_python_value.hpp>
#include <type_traits>

/**
 * Policy that can convert to return type to a super type
 */

namespace Mantid {
namespace PythonInterface {
namespace Policies {
// Utility code to help out
namespace {

//-----------------------------------------------------------------------
// Polciy implementation
//-----------------------------------------------------------------------

// The entry point for the policy is in the struct AsType below. It does
// a check as to whether the return type is valid, if so it forwards the
// call to this struct
template <typename ReturnType, typename InputType> struct AsTypeImpl {

  inline PyObject *operator()(const InputType &p) const {
    using namespace boost::python;
    return to_python_value<ReturnType>()(ReturnType(p));
  }

  inline PyTypeObject const *get_pytype() const {
    using namespace boost::python;
    return converter::registered<ReturnType>::converters
        .to_python_target_type();
  }
};

// Error handler for shared pointer types. If return type is wrong then user
// sees the name of this
// class in the output, which hopefully gives a clue as to what is going on
template <typename T>
struct AsType_Requires_New_Type_Automatically_Convertible_To_Original {};

} // namespace

/**
 * Implements the AsType policy.
 */
template <class ReturnType> struct AsType {
  template <class InputType> struct apply {
    // Deduce if type is correct for policy, needs to be convertible to
    // ReturnType
    using type = typename boost::mpl::if_c<
        std::is_convertible<InputType, ReturnType>::value,
        AsTypeImpl<ReturnType, InputType>,
        AsType_Requires_New_Type_Automatically_Convertible_To_Original<
            InputType>>::type;
  };
};

} // namespace Policies
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_ASTYPE_H */
