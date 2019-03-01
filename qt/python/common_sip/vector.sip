// Acquired and modified based on the version from the wxpy project at
// https://raw.githubusercontent.com/ifwe/wxpy/master/src/vector.sip
// on 9th April 2018.
//
// std::vector<std::string> specialization is based off string.sip.

// SIP support for std::vector
// by Giovanni Bajo <rasky <at> develer.com>
// Public domain

// ****************************************************
// SIP generic implementation for std::vector<>
// ****************************************************
// ALas, this template-based generic implementation is valid only
// if the element type is a SIP-wrapped type. For basic types (int, double, etc.)
// we are forced to cut & paste to provide a specialization.
%Include string.sip

%ModuleCode
#include "SIPVector.h"
%End

template<TYPE>
%MappedType std::vector<TYPE>
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  return vectorToPythonList(*sipCpp, [&](TYPE item) -> PyObject* {
    auto *heapCopy = new TYPE(item);
    return sipConvertFromInstance(heapCopy, sipClass_TYPE, sipTransferObj);
  });
%End

%ConvertToTypeCode
  if (!sipIsErr) {
    // Check if type is compatible
    return isIterable(sipPy);
  } else {
    auto cppValue = pythonListToVector<TYPE>(sipPy, [&](PyObject* pyItem) -> boost::optional<TYPE> {
      if (!sipCanConvertToInstance(pyItem, sipClass_TYPE, SIP_NOT_NONE)) {
        PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to TYPE");
        return boost::none;
      } else {
        int state;
        auto* cppItemPtr = reinterpret_cast<TYPE*>(
           sipConvertToInstance(pyItem, sipClass_TYPE, 0, SIP_NOT_NONE, &state, sipIsErr));

        sipReleaseInstance(cppItemPtr, sipClass_TYPE, state);
        return asOptional(sipIsErr, cppItemPtr);
      }
    });
    return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
  }
%End
};

template<TYPE>
%MappedType boost::optional<std::vector<TYPE>>
{
%TypeHeaderCode
#include <vector>
#include <boost/optional.hpp>
%End

%ConvertFromTypeCode
  return optionalToPyObject(*sipCpp, [&](std::vector<TYPE> const& vector) -> PyObject* {
    return vectorToPythonList(vector, [&](TYPE const& item) -> PyObject* {
      auto *heapCopy = new TYPE(item);
      return sipConvertFromInstance(heapCopy, sipClass_TYPE, sipTransferObj);
    });
  });
%End

%ConvertToTypeCode
    if (!sipIsErr) {
      // Check if type is compatible
      return isIterable(sipPy) || sipPy == Py_None;
    } else {
      auto cppValue = pythonObjectToOptional<std::vector<TYPE>>(sipPy, [&](PyObject* pythonList) -> boost::optional<std::vector<TYPE>> {
        return pythonListToVector<TYPE>(pythonList, [&](PyObject* pyItem) -> boost::optional<TYPE> {
          if (!sipCanConvertToInstance(pyItem, sipClass_TYPE, SIP_NOT_NONE)) {
            PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to TYPE");
            return boost::none;
          } else {
            int state;
            auto* cppItemPtr = reinterpret_cast<TYPE*>(
               sipConvertToInstance(pyItem, sipClass_TYPE, 0, SIP_NOT_NONE, &state, sipIsErr));

            sipReleaseInstance(cppItemPtr, sipClass_TYPE, state);
            return asOptional(sipIsErr, cppItemPtr);
          }
        });
      });
      return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
    }
%End
};

// ****************************************************
// Specialization for std::vector<double>
// ****************************************************
%MappedType std::vector<double>
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  return vectorToPythonList(*sipCpp, [](double item) -> PyObject* {
    return PyFloat_FromDouble(item) ;
  });
%End

%ConvertToTypeCode
  if (!sipIsErr) {
    // Check if type is compatible
    return isIterable(sipPy) || sipPy == Py_None;
  } else {
    auto cppValue = pythonListToVector<double>(sipPy, [&](PyObject* pyItem) -> boost::optional<double> {
      if (!PyNumber_Check(pyItem)) {
        PyErr_Format(PyExc_TypeError, "object in iterable is not a number");
        return boost::none;
      } else {
        return PyFloat_AsDouble(pyItem);
      }
    });
    return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
  }
%End
};

// ****************************************************
// Specialization for std::vector<int>
// ****************************************************

%MappedType std::vector<int>
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  return vectorToPythonList(*sipCpp, [](int item) -> PyObject* {
    return FROM_LONG(item);
  });
%End

%ConvertToTypeCode
  if (!sipIsErr) {
    // Check if type is compatible
    return isIterable(sipPy) || sipPy == Py_None;
  } else {
    auto cppValue = pythonListToVector<int>(sipPy, [&](PyObject* pyItem) -> boost::optional<int> {
      if (!INT_CHECK(pyItem)) {
        PyErr_Format(PyExc_TypeError, "object in iterable is not an int");
        return boost::none;
      } else {
        return static_cast<int>(TO_LONG(pyItem));
      }
    });
    return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
  }
%End
};


// ****************************************************
// Specialization for std::vector<unsigned int>
// ****************************************************

%MappedType std::vector<unsigned int>
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  return vectorToPythonList(*sipCpp, [](unsigned int item) -> PyObject* {
    return FROM_LONG(item);
  });
%End

%ConvertToTypeCode
  if (!sipIsErr) {
    // Check if type is compatible
    return isIterable(sipPy) || sipPy == Py_None;
  } else {
    auto cppValue = pythonListToVector<unsigned int>(sipPy, [&](PyObject* pyItem) -> boost::optional<unsigned int> {
      if (!INT_CHECK(pyItem)) {
        PyErr_Format(PyExc_TypeError, "object in iterable is not an int");
        return boost::none;
      } else {
        return static_cast<unsigned int>(TO_LONG(pyItem));
      }
    });
    return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
  }
%End
};

// ****************************************************
// Specialization for std::vector<std::string>
// ****************************************************

%MappedType std::vector<std::string>
{
%TypeHeaderCode
#include <vector>
#include <string>
%End

%ConvertFromTypeCode
  return vectorToPythonList(*sipCpp, &stdStringToPythonString);
%End

%ConvertToTypeCode
  if (!sipIsErr) {
    // Check if type is compatible
    return isIterable(sipPy) || sipPy == Py_None;
  } else {
    auto cppValue = pythonListToVector<std::string>(sipPy, [&](PyObject* pyItem) -> boost::optional<std::string> {
      auto maybeValue = pythonStringToStdString(pyItem);
      return typeErrorIfNoneElseValue(maybeValue, "object in iterable is not a string.");
    });
    return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
  }
%End
};

// ****************************************************
// Specialization for std::vector<std::vector<TYPE>>
// ****************************************************

template<TYPE>
%MappedType std::vector<std::vector<TYPE>>
{
%TypeHeaderCode
#include <vector>
#include <memory>
%End

%ConvertFromTypeCode
  return vectorToPythonList(*sipCpp, [&](std::vector<TYPE> const& item) -> PyObject* {
    return vectorToPythonList(item, [&](TYPE const& item) -> PyObject* {
      auto *heapCopy = new TYPE(item);
      return sipConvertFromInstance(heapCopy, sipClass_TYPE, sipTransferObj);
    });
  });
%End

%ConvertToTypeCode
  if (!sipIsErr) {
    // Check if type is compatible
    return isIterable(sipPy);
  } else {
    auto cppValue = pythonListToVector<std::vector<TYPE>>(sipPy, [&](PyObject* pyItem) -> boost::optional<std::vector<TYPE>> {
      return pythonListToVector<TYPE>(pyItem, [&](PyObject* pyInnerItem) -> boost::optional<TYPE> {
        if (!sipCanConvertToInstance(pyInnerItem, sipClass_TYPE, SIP_NOT_NONE)) {
          PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to the damn TYPE");
          return boost::none;
        } else {
          int state;
          auto* cppItemPtr = reinterpret_cast<TYPE*>(
             sipConvertToInstance(pyInnerItem, sipClass_TYPE, 0, SIP_NOT_NONE, &state, sipIsErr));
          sipReleaseInstance(cppItemPtr, sipClass_TYPE, state);
          return asOptional(sipIsErr, cppItemPtr);
        }
      });
    });
    return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
  }
%End
};

// ****************************************************
// Specialization for std::vector<std::vector<TYPE>>
// ****************************************************

template<TYPE>
%MappedType boost::optional<std::vector<std::vector<TYPE>>>
{
%TypeHeaderCode
#include <vector>
#include <string>
#include <memory>
%End

%ConvertFromTypeCode
  return optionalToPyObject(*sipCpp, [&, sipTransferObj](std::vector<std::vector<TYPE>> const& vector) -> PyObject* {
    return vectorToPythonList(vector, [&](std::vector<TYPE> const& item) -> PyObject* {
      return vectorToPythonList(item, [&](TYPE const& item) -> PyObject* {
        auto *heapCopy = new TYPE(item);
        return sipConvertFromInstance(heapCopy, sipClass_TYPE, sipTransferObj);
      });
    });
  });
%End

%ConvertToTypeCode

  if (!sipIsErr) {
    // Check if type is compatible
    return isIterable(sipPy) || sipPy == Py_None;
  } else {
    auto cppValue = pythonObjectToOptional<std::vector<std::vector<TYPE>>>(sipPy,
        [&](PyObject* pythonList) -> boost::optional<std::vector<std::vector<TYPE>>> {
          return pythonListToVector<std::vector<TYPE>>(pythonList, [&](PyObject* pythonInnerList) -> boost::optional<std::vector<TYPE>> {
            return pythonListToVector<TYPE>(pythonInnerList, [&](PyObject* pyItem) -> boost::optional<TYPE> {
              if (!sipCanConvertToInstance(pyItem, sipClass_TYPE, SIP_NOT_NONE)) {
                PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to TYPE");
                return boost::none;
              } else {
                int state;
                auto* cppItemPtr = reinterpret_cast<TYPE*>(
                   sipConvertToInstance(pyItem, sipClass_TYPE, 0, SIP_NOT_NONE, &state, sipIsErr));

                sipReleaseInstance(cppItemPtr, sipClass_TYPE, state);
                return asOptional(sipIsErr, cppItemPtr);
              }
            });
          });
        });
    return transferToSip(cppValue, sipCppPtr, sipIsErr, sipGetState(sipTransferObj));
  }

%End
};
