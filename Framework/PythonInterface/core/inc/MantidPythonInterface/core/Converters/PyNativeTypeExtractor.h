// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>
#include <variant>
#include <vector>

#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/variant.hpp>

#include <exception>

namespace Mantid::PythonInterface {

struct PyNativeTypeExtractor {
  using PythonOutputT =
      boost::make_recursive_variant<bool, long, double, std::string, std::vector<boost::recursive_variant_>>::type;

  static PythonOutputT convert(const boost::python::object &obj) {
    using namespace boost::python;
    PyObject *rawptr = obj.ptr();
    PythonOutputT out;

    if (PyList_Check(rawptr)) {
      out = handleList(obj);
    } else if (PyBool_Check(rawptr)) {
      out = extract<bool>(obj);
    } else if (PyFloat_Check(rawptr)) {
      out = extract<double>(obj);
    } else if (PyLong_Check(rawptr)) {
      out = extract<long>(obj);
    } else if (PyUnicode_Check(rawptr)) {
      out = extract<std::string>(obj);
    } else {
      throw std::invalid_argument("Unrecognised Python type");
    }
    return out;
  }

private:
  static PythonOutputT handleList(const boost::python::object &obj) {
    auto rawptr = obj.ptr();
    auto n = PyList_Size(rawptr);
    auto vec = std::vector<PythonOutputT>();
    vec.reserve(n);
    for (Py_ssize_t i = 0; i < n; ++i) {
      vec.emplace_back(convert(obj[i]));
    }
    return vec;
  }
};

class IPyTypeVisitor : public boost::static_visitor<> {
public:
  /**
   * Dynamically dispatches to overloaded operator depending on the underlying type.
   * This also handles cases such as std::vector<T>, or nested lists, which will be flattened
   * by invoking the operator each time for each elem.
   * This assumes all element in a list matches the first element type found in that list,
   * where this is not true, the element will be cast to the first element type.
   *
   * Note: you will need to include
   * using Mantid::PythonInterface::IPyTypeVisitor::operator();
   */

  virtual ~IPyTypeVisitor() = default;
  virtual void operator()(bool value) const = 0;
  virtual void operator()(long value) const = 0;
  virtual void operator()(double value) const = 0;
  virtual void operator()(std::string) const = 0;

  virtual void operator()(std::vector<bool>) const = 0;
  virtual void operator()(std::vector<long>) const = 0;
  virtual void operator()(std::vector<double>) const = 0;
  virtual void operator()(std::vector<std::string>) const = 0;

  void operator()(std::vector<PyNativeTypeExtractor::PythonOutputT> const &values) const {
    if (values.size() == 0)
      return;
    const auto &elemType = values[0].type();

    // We must manually dispatch for container types, as boost will try
    // to recurse down to scalar values.
    if (elemType == typeid(bool)) {
      applyVectorProp<bool>(values);
    } else if (elemType == typeid(double)) {
      applyVectorProp<double>(values);
    } else if (elemType == typeid(long)) {
      applyVectorProp<long>(values);
    } else if (elemType == typeid(std::string)) {
      applyVectorProp<std::string>(values);
    } else {
      // Recurse down
      for (const auto &val : values) {
        boost::apply_visitor(*this, val);
      }
    }
  }

private:
  template <typename ScalarT>
  void applyVectorProp(const std::vector<Mantid::PythonInterface::PyNativeTypeExtractor::PythonOutputT> &values) const {
    std::vector<ScalarT> propVals;
    propVals.reserve(values.size());

    // Explicitly copy so we don't have to think about Python lifetimes with refs
    std::transform(values.cbegin(), values.cend(), std::back_inserter(propVals),
                   [](const Mantid::PythonInterface::PyNativeTypeExtractor::PythonOutputT &varadicVal) {
                     return boost::get<ScalarT>(varadicVal);
                   });
    this->operator()(std::move(propVals));
  }
};

} // namespace Mantid::PythonInterface
