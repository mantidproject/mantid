// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/VersionCompat.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

bool isIterable(PyObject *iterable) {
  PyObject *iterator = PyObject_GetIter(iterable);
  bool isIterable = (iterator != nullptr);
  Py_XDECREF(iterator);
  return isIterable;
}

template <typename T>
std::optional<T> typeErrorIfNoneElseValue(std::optional<T> const &maybeValue, std::string const &errorMessage) {
  if (!maybeValue.has_value())
    PyErr_SetString(PyExc_TypeError, errorMessage.c_str());
  return maybeValue;
}

template <typename T, typename ConversionFunction>
std::optional<std::optional<T>> pythonObjectToOptional(PyObject *object, ConversionFunction pyObjectAsValue) {
  if (object == Py_None)
    return std::nullopt;
  else
    return pyObjectAsValue(object);
}

template <typename T, typename ConversionFunction>
PyObject *optionalToPyObject(std::optional<T> const &item, ConversionFunction valueAsPyObject) {
  if (item.has_value()) {
    return valueAsPyObject(item.value());
  } else {
    return Py_None;
  }
}

template <typename T, typename ConversionFunction>
PyObject *vectorToPythonList(std::vector<T> const &vector, ConversionFunction itemToPyObject) {
  PyObject *pythonList = PyList_New(vector.size());
  if (pythonList != nullptr) {
    for (int i = 0; i < static_cast<int>(vector.size()); ++i) {
      auto *pyItem = itemToPyObject(vector.at(i));
      if (pyItem != nullptr) {
        PyList_SET_ITEM(pythonList, i, pyItem);
      } else {
        Py_DECREF(pythonList);
        return nullptr;
      }
    }
    return pythonList;
  } else {
    PyErr_Format(PyExc_TypeError, "Failed to allocate new python list.");
    return nullptr;
  }
}

template <typename T, typename ConversionFunction>
std::optional<std::vector<T>> pythonListToVector(PyObject *pythonList, ConversionFunction pyObjectToItem) {
  auto length = static_cast<int>(PyObject_Size(pythonList));
  PyObject *iterator = PyObject_GetIter(pythonList);
  if (iterator != nullptr) {
    auto cppVector = std::vector<T>();
    cppVector.reserve(length);
    PyObject *pythonItem = nullptr;
    while ((pythonItem = PyIter_Next(iterator))) {
      auto item = pyObjectToItem(pythonItem);
      if (item.has_value())
        cppVector.emplace_back(std::move(item.value()));
      else {
        Py_DECREF(pythonItem);
        Py_DECREF(iterator);
        return std::nullopt;
      }
      Py_DECREF(pythonItem);
    }
    Py_DECREF(iterator);
    return cppVector;
  } else {
    return std::nullopt;
  }
}

template <typename T> int transferToSip(std::optional<T> const &cppValue, T **sipCppPtr, int *sipIsErr, int sipState) {
  if (cppValue.has_value()) {
    auto heapValue = ::std::make_unique<T>(std::move(cppValue.value()));
    *sipCppPtr = heapValue.release();
    return sipState;
  } else {
    *sipIsErr = 1;
    return 0;
  }
}

template <typename T> std::optional<T> asOptional(int *sipIsErr, T *sipCppPtr) {
  if (*sipIsErr)
    return std::nullopt;
  else
    return *sipCppPtr;
}

inline std::optional<std::string> pythonStringToStdString(PyObject *pyString) {
  if (PyBytes_Check(pyString)) {
    return std::string(PyBytes_AsString(pyString));
  } else if (STR_CHECK(pyString)) {
    return std::string(TO_CSTRING(pyString));
  } else {
    return std::nullopt;
  }
}

PyObject *stdStringToPythonString(std::string const &cppString) {
  if (auto *utf8String = PyUnicode_DecodeUTF8(cppString.c_str(), cppString.length(), nullptr)) {
    return utf8String;
  } else {
    PyErr_Clear();
    return FROM_CSTRING(cppString.c_str());
  }
}
