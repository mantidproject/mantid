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
  ExtractSharedPtr(const boost::python::object &pyvalue);
  bool check() const;
  const std::shared_ptr<T> operator()() const;

private:
  std::shared_ptr<T> m_value;
};

/**
 * @param pyvalue Python object from which to extract
 */
template <typename T>
ExtractSharedPtr<T>::ExtractSharedPtr(const boost::python::api::object &pyvalue)
    : m_value() {
  // Here we assume we want to extract out an existing shared_ptr from a Python
  // object if one exists. By default, using extract on a shared_ptr/weak_ptr
  // type causes boost::python to create a *new* shared_ptr object managing the
  // original resource. This then leads to bad behaviour with two objects
  // managing the same memory.

  // Test for a weak pointer first
  using boost::python::extract;
  if (extract<std::weak_ptr<T> &> extractWeakRef(pyvalue);
      extractWeakRef.check()) {
    m_value = extractWeakRef().lock();
  } else if (extract<std::shared_ptr<T> &> extractSharedRef(pyvalue);
             extractSharedRef.check()) {
    m_value = extractSharedRef();
  }
}

/**
 * Check whether the extract can pull out the workspace type
 * @return True if it can be converted, false otherwise
 */
template <typename T> bool ExtractSharedPtr<T>::check() const {
  return m_value.get() != nullptr;
}

/**
 * @return The extracted shared_ptr or throws std::invalid_argument
 */
template <typename T>
const std::shared_ptr<T> ExtractSharedPtr<T>::operator()() const {
  if (check()) {
    return m_value;
  } else {
    throw std::invalid_argument(
        "Unable to extract shared_ptr from Python object");
  }
}

} // namespace Mantid::PythonInterface
