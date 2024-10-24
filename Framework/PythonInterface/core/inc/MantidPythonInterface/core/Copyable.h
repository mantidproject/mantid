// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>
#include <boost/python/manage_new_object.hpp>
namespace Mantid {
namespace PythonInterface {

// Implementation based on
// https://mail.python.org/pipermail/cplusplus-sig/2009-May/014505.html
namespace bp = boost::python;

/**
 * Create a new object handle around arg. Caller is owner.
 */
template <typename T> inline PyObject *managingPyObject(T *p) {
  return typename bp::manage_new_object::apply<T *>::type()(p);
}

/**
 * Create a shallow copy of type Copyable providing Copyable is newable and has
 * a public copy constructor
 */
template <typename Copyable> boost::python::object generic__copy__(const bp::object &copyable) {
  Copyable *newCopyable(new Copyable(bp::extract<const Copyable &>(copyable)));
  bp::object result(bp::detail::new_reference(managingPyObject(newCopyable)));

  bp::extract<bp::dict>(result.attr("__dict__"))().update(copyable.attr("__dict__"));

  return result;
}

/**
 * Create a deep copy of type Copyable providing Copyable is newable and has a
 * public copy constructor
 */
template <typename Copyable> bp::object generic__deepcopy__(const bp::object &copyable, bp::dict &memo) {
  bp::object copyMod = bp::import("copy");
  bp::object deepcopy = copyMod.attr("deepcopy");

  Copyable *newCopyable(new Copyable(bp::extract<const Copyable &>(copyable)));
  bp::object result(bp::detail::new_reference(managingPyObject(newCopyable)));

  // HACK: copyableId shall be the same as the result of id(copyable)
  auto copyableId = (std::ptrdiff_t)(copyable.ptr());
  memo[copyableId] = result;

  bp::extract<bp::dict>(result.attr("__dict__"))().update(
      deepcopy(bp::extract<bp::dict>(copyable.attr("__dict__"))(), memo));

  return result;
}
} // namespace PythonInterface
} // namespace Mantid
