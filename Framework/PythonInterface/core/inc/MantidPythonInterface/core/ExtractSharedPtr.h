// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <boost/python/extract.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace Mantid::PythonInterface {

template <typename T> struct ExtractSharedPtr {
  explicit ExtractSharedPtr(const boost::python::object &pyvalue) noexcept : ExtractSharedPtr<T>(pyvalue.ptr()) {}
  explicit ExtractSharedPtr(PyObject *pyvalue) noexcept;
  inline bool check() const noexcept { return m_value.get() != nullptr; }
  const std::shared_ptr<T> operator()() const;

private:
  std::shared_ptr<T> m_value;
};

/**
 * @param pyvalue Python object from which to attempt extraction of an object of
 * type T
 */
template <typename T> ExtractSharedPtr<T>::ExtractSharedPtr(PyObject *pyvalue) noexcept : m_value() {
  // Here we assume we want to extract out an existing shared_ptr from a Python
  // object if one exists. Naievly one would just do extract<std::shared_ptr<T>>
  // but this will create a second shared_ptr managing the same resource and
  // undefined behaviour when both come to try and delete the same object.
  //
  // The correct course of action is to extract a reference to shared_ptr<T>
  // and this will fail if the object is not a shared_ptr. We also deal with
  // the case where a weak_ptr may have been handed out by first trying to
  // extract a reference to weak_ptr<T> and falling back to a reference to
  // shared_ptr<T> if this fails.

  using boost::python::extract;
  if (extract<std::weak_ptr<T> &> extractWeakRef(pyvalue); extractWeakRef.check()) {
    m_value = extractWeakRef().lock();
  } else if (extract<std::shared_ptr<T> &> extractSharedRef(pyvalue); extractSharedRef.check()) {
    m_value = extractSharedRef();
  }
}

/**
 * @return The extracted shared_ptr or throws std::invalid_argument
 */
template <typename T> const std::shared_ptr<T> ExtractSharedPtr<T>::operator()() const {
  if (check()) {
    return m_value;
  } else {
    throw std::invalid_argument("Unable to extract shared_ptr from Python object");
  }
}

} // namespace Mantid::PythonInterface
