// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_REMOVECONST_H_
#define MANTID_PYTHONINTERFACE_REMOVECONST_H_

#include <boost/python/detail/prefix.hpp>
#include <boost/python/to_python_value.hpp>

#include <boost/mpl/and.hpp>

#include <boost/shared_ptr.hpp>

#include <type_traits>

/**
 * A bug in earlier boost versions, <= boost 1.41, means that
 * using boost::register_ptr_to_python for const pointers gives a compiler
 *error.
 * This means that automatic conversion from const to non-const pointers
 * in return values and function arguments is suppressed.
 *
 * These policies simply removes the const from the returned pointer
 * and emits the corresponding Python object using the registered converters.
 * Two policies are defined:
 *  - RemoveConst: Bare pointers, return type must be "const T*"
 *  - RemoveConstSharedPtr: SharedPtr to const object, return type must be
 *boost::shared_ptr<const T>
 */

namespace Mantid {
namespace PythonInterface {
namespace Policies {
// Utility code to help out
namespace {
//-----------------------------------------------------------------------
// MPL helper structs
//-----------------------------------------------------------------------
/// MPL struct to figure out if a type is a boost::shared_ptr<const T>
/// The general one inherits from std::false_type
template <typename T> struct IsConstSharedPtr : std::false_type {};

/// Specialization for boost::shared_ptr<const T> types to inherit from
/// std::true_type
template <typename T>
struct IsConstSharedPtr<boost::shared_ptr<const T>> : std::true_type {};

//-----------------------------------------------------------------------
// Polciy implementations
//-----------------------------------------------------------------------

// The entry point for the policy is in the struct RemoveConst below. It does
// a check as to whether the return type is valid, if so it forwards the
// call to this struct
template <typename ConstPtrType> struct RemoveConstImpl {
  // Remove the pointer type to leave value type
  using ValueType = typename std::remove_pointer<ConstPtrType>::type;
  // Remove constness
  using NonConstValueType = typename std::remove_const<ValueType>::type;

  inline PyObject *operator()(const ConstPtrType &p) const {
    using namespace boost::python;
    return to_python_value<NonConstValueType *>()(
        const_cast<NonConstValueType *>(p));
  }

  inline PyTypeObject const *get_pytype() const {
    using namespace boost::python;
    return converter::registered<NonConstValueType *>::converters
        .to_python_target_type();
  }
};

// Error handler for raw pointer types. If return type is wrong then user sees
// the name of this
// class in the output, which hopefully gives a clue as to what is going on
template <typename T> struct RemoveConst_Requires_Pointer_Return_Value {};

// The entry point for the policy is in the struct RemoveConstSharedPtr below.
// It does
// a check as to whether the return type is valid, if so it forwards the
// call to this struct
template <typename ConstSharedPtr> struct RemoveConstSharedPtrImpl {
  using ConstElementType = typename ConstSharedPtr::element_type;
  using NonConstElementType =
      typename std::remove_const<ConstElementType>::type;
  using NonConstSharedPtr = typename boost::shared_ptr<NonConstElementType>;

  inline PyObject *operator()(const ConstSharedPtr &p) const {
    using namespace boost::python;
    return to_python_value<NonConstSharedPtr>()(
        boost::const_pointer_cast<NonConstElementType>(p));
  }

  inline PyTypeObject const *get_pytype() const {
    using namespace boost::python::converter;
    return registered<NonConstSharedPtr>::converters.to_python_target_type();
  }
};

// Error handler for shared pointer types. If return type is wrong then user
// sees the name of this
// class in the output, which hopefully gives a clue as to what is going on
template <typename T>
struct RemoveConstSharedPtr_Requires_SharedPtr_Const_T_Pointer_Return_Value {};

} // namespace

/**
 * Implements the RemoveConst policy.
 */
struct RemoveConst {
  template <class T> struct apply {
    // Deduce if type is correct for policy, needs to be a "T*"
    using type = typename boost::mpl::if_c<
        std::is_pointer<T>::value, RemoveConstImpl<T>,
        RemoveConst_Requires_Pointer_Return_Value<T>>::type;
  };
};

/**
 * Implements the RemoveConstSharedPtr policy.
 */
struct RemoveConstSharedPtr {
  template <class T> struct apply {
    // Deduce if type is correct for policy, needs to be a
    // "boost::shared_ptr<T>"
    using type = typename boost::mpl::if_c<
        IsConstSharedPtr<T>::value, RemoveConstSharedPtrImpl<T>,
        RemoveConstSharedPtr_Requires_SharedPtr_Const_T_Pointer_Return_Value<
            T>>::type;
  };
};

} // namespace Policies
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_REMOVECONST_H_REMOVECONST_H_ */