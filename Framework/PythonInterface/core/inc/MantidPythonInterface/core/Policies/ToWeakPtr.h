// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_TOWEAKPTR_H_
#define MANTID_PYTHONINTERFACE_TOWEAKPTR_H_

#include <boost/python/detail/prefix.hpp>
#include <boost/python/to_python_value.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Policies {

namespace {
//-----------------------------------------------------------------------
// MPL helper structs
//-----------------------------------------------------------------------
/// MPL struct to figure out if a type is a boost::shared_ptr<T>
/// The general one inherits from boost::false_type
template <typename T> struct IsSharedPtr : boost::false_type {};

/// Specialization for boost::shared_ptr<const T> types to inherit from
/// boost::true_type
template <typename T>
struct IsSharedPtr<boost::shared_ptr<T>> : boost::true_type {};

//-----------------------------------------------------------------------
// Policy implementation
//-----------------------------------------------------------------------
/**
 * Constructs a boost::weak_ptr around the incoming boost::shared_ptr
 */
template <typename ArgType> struct ToWeakPtrImpl {
  // Useful types
  using PointeeType = typename ArgType::element_type;
  using WeakPtr = boost::weak_ptr<PointeeType>;

  inline PyObject *operator()(const ArgType &p) const {
    if (!p)
      Py_RETURN_NONE;
    return boost::python::to_python_value<WeakPtr>()(WeakPtr(p));
  }

  inline PyTypeObject const *get_pytype() const {
    return boost::python::converter::registered<WeakPtr>::converters
        .to_python_target_type();
  }
};

//-----------------------------------------------------------------------
// Error handler
//-----------------------------------------------------------------------
template <typename T> struct ToWeakPtr_Requires_Shared_Ptr_Return_Value {};
} // namespace

/**
 * Implements the ToWeakPtr policy as required by boost.python
 */
struct ToWeakPtr {
  template <class T> struct apply {
    // Deduce if type is correct for policy
    using type = typename boost::mpl::if_c<
        IsSharedPtr<T>::value, ToWeakPtrImpl<T>,
        ToWeakPtr_Requires_Shared_Ptr_Return_Value<T>>::type;
  };
};

} // namespace Policies
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_REMOVECONST_H_REMOVECONST_H_ */
