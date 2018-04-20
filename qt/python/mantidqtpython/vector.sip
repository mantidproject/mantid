// Acquired from the wxpy project at
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

template<TYPE>
%MappedType std::vector<TYPE>
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
    PyObject *l = PyList_New(sipCpp -> size());

    // Create the Python list of the correct length.
    if (!l)
        return NULL;

    // Go through each element in the C++ instance and convert it to a
    // wrapped P2d.
    for (int i = 0; i < (int)sipCpp->size(); ++i) {
        TYPE *cpp = new TYPE(sipCpp->at(i));
        PyObject *pobj = sipConvertFromInstance(cpp, sipClass_TYPE, sipTransferObj);

        // Get the Python wrapper for the Type instance, creating a new
        // one if necessary, and handle any ownership transfer.
        if (!pobj) {
            // There was an error so garbage collect the Python list.
            Py_DECREF(l);
            return NULL;
        }

        // Add the wrapper to the list.
        PyList_SET_ITEM(l, i, pobj);
    }

    // Return the Python list.
    return l;
%End

%ConvertToTypeCode
    // Check if type is compatible
    if (!sipIsErr) {
        // Must be any iterable
        PyObject *i = PyObject_GetIter(sipPy);
        bool iterable = (i != NULL);
        Py_XDECREF(i);
        return iterable;
    }

    // Iterate over the object
    PyObject *iterator = PyObject_GetIter(sipPy);
    PyObject *item;

    std::vector<TYPE> *V = new std::vector<TYPE>();

    while ((item = PyIter_Next(iterator)))
    {
        if (!sipCanConvertToInstance(item, sipClass_TYPE, SIP_NOT_NONE)) {
            PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to TYPE");
            *sipIsErr = 1;
            break;
        }

        int state;
        TYPE* p = reinterpret_cast<TYPE*>(
             sipConvertToInstance(item, sipClass_TYPE, 0, SIP_NOT_NONE, &state, sipIsErr));

        if (!*sipIsErr)
            V->push_back(*p);

        sipReleaseInstance(p, sipClass_TYPE, state);
        Py_DECREF(item);
    }

    Py_DECREF(iterator);

    if (*sipIsErr) {
        delete V;
        return 0;
    }

    *sipCppPtr = V;
    return sipGetState(sipTransferObj);
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
    PyObject *l;

    // Create the Python list of the correct length.
    if ((l = PyList_New(sipCpp -> size())) == NULL)
        return NULL;

    // Go through each element in the C++ instance and convert it to a
    // wrapped object.
    for (int i = 0; i < (int)sipCpp -> size(); ++i)
    {
        // Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyFloat_FromDouble(sipCpp -> at(i)));
    }

    // Return the Python list.
    return l;
%End

%ConvertToTypeCode
    // Check if type is compatible
    if (sipIsErr == NULL)
    {
        // Must be any iterable
        PyObject *i = PyObject_GetIter(sipPy);
        bool iterable = (i != NULL);
        Py_XDECREF(i);
        return iterable;
    }

    // Iterate over the object
    PyObject *iterator = PyObject_GetIter(sipPy);
    PyObject *item;

    // Maximum number of elements
    Py_ssize_t len = PyObject_Size(sipPy);
    std::vector<double> *V = new std::vector<double>();
    V->reserve(len);

    if (len)
    {
        while ((item = PyIter_Next(iterator)))
        {
            if (!PyNumber_Check(item))
            {
                PyErr_Format(PyExc_TypeError, "object in iterable is not a number");
                *sipIsErr = 1;
                break;
            }

            PyObject *f = PyNumber_Float(item);
            V->push_back(PyFloat_AsDouble(f));

            Py_DECREF(f);
            Py_DECREF(item);
        }

        Py_DECREF(iterator);

        if (*sipIsErr)
        {
            delete V;
            return 0;
        }
    }

    *sipCppPtr = V;
    return sipGetState(sipTransferObj);
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
    PyObject *l;

    // Create the Python list of the correct length.
    if ((l = PyList_New((SIP_SSIZE_T)sipCpp -> size())) == NULL)
        return NULL;

    // Go through each element in the C++ instance and convert it to a
    // wrapped object.
    for (int i = 0; i < (int)sipCpp -> size(); ++i)
    {
        // Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyInt_FromLong(sipCpp -> at(i)));
    }

    // Return the Python list.
    return l;
%End

%ConvertToTypeCode
    // Check if type is compatible
    if (sipIsErr == NULL)
    {
        // Must be any iterable
        PyObject *i = PyObject_GetIter(sipPy);
        bool iterable = (i != NULL);
        Py_XDECREF(i);
        return iterable;
    }

    // Iterate over the object
    PyObject *iterator = PyObject_GetIter(sipPy);
    PyObject *item;

    // Maximum number of elements
    int len = static_cast<int>(PyObject_Size(sipPy));
    std::vector<int> *V = new std::vector<int>();
    V->reserve(len);

    if (len)
    {
        while ((item = PyIter_Next(iterator)))
        {
            if (!PyInt_Check(item))
            {
                PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to float");
                *sipIsErr = 1;
                break;
            }

            int val = static_cast<int>(PyInt_AsLong(item));
            V->push_back(val);

            Py_DECREF(item);
        }

        Py_DECREF(iterator);

        if (*sipIsErr)
        {
            delete V;
            return 0;
        }
    }

    *sipCppPtr = V;
    return sipGetState(sipTransferObj);
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
    PyObject *l;

    // Create the Python list of the correct length.
    if ((l = PyList_New(sipCpp -> size())) == NULL)
        return NULL;

    // Go through each element in the C++ instance and convert it to a
    // wrapped object.
    for (int i = 0; i < (int)sipCpp -> size(); ++i)
    {
        // Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyInt_FromLong(sipCpp -> at(i)));
    }

    // Return the Python list.
    return l;
%End

%ConvertToTypeCode
    // Check if type is compatible
    if (sipIsErr == NULL)
    {
        // Must be any iterable
        PyObject *i = PyObject_GetIter(sipPy);
        bool iterable = (i != NULL);
        Py_XDECREF(i);
        return iterable;
    }

    // Iterate over the object
    PyObject *iterator = PyObject_GetIter(sipPy);
    PyObject *item;

    // Maximum number of elements
    Py_ssize_t size = PyObject_Size(sipPy);
    if (size == -1) {
        Py_DECREF(iterator);
        return 0;
    }

    unsigned int len = static_cast<unsigned int>(size);
    std::vector<unsigned int> *V = new std::vector<unsigned int>();
    V->reserve(len);

    if (len)
    {
        while ((item = PyIter_Next(iterator)))
        {
            if (!PyInt_Check(item))
            {
                PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to float");
                *sipIsErr = 1;
                break;
            }

            unsigned int val = static_cast<unsigned int>(PyInt_AsLong(item));
            V->push_back(val);

            Py_DECREF(item);
        }

        Py_DECREF(iterator);

        if (*sipIsErr)
        {
            delete V;
            return 0;
        }
    }

    *sipCppPtr = V;
    return sipGetState(sipTransferObj);
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
    PyObject *l;

    // Create the Python list of the correct length.
    if ((l = PyList_New((SIP_SSIZE_T)sipCpp -> size())) == NULL)
        return NULL;

    // Go through each element in the C++ instance and convert it to a
    // wrapped object.
    for (int i = 0; i < (int)sipCpp -> size(); ++i)
    {
        PyObject* newstring;
        newstring = PyUnicode_DecodeUTF8(sipCpp->at(i).c_str(), sipCpp->at(i).length(), NULL);
        if(newstring == NULL) {
            PyErr_Clear();
            newstring = PyString_FromString(sipCpp->at(i).c_str());
        }
        // Add the wrapper to the list.
        PyList_SET_ITEM(l, i, newstring);
    }

    // Return the Python list.
    return l;
%End

%ConvertToTypeCode
    // Check if type is compatible
    if (sipIsErr == NULL) {
        // Must be any iterable
        PyObject *i = PyObject_GetIter(sipPy);
        bool iterable = (i != NULL);
        Py_XDECREF(i);

        return iterable;
    }

    // Iterate over the object
    PyObject *iterator = PyObject_GetIter(sipPy);
    PyObject *item;

    // Maximum number of elements
    int len = static_cast<int>(PyObject_Size(sipPy));
    std::vector<std::string> *V = new std::vector<std::string>();
    V->reserve(len);

    if (len)
    {
        while ((item = PyIter_Next(iterator)))
        {
            if (item != Py_None && !PyString_Check(item) && !PyUnicode_Check(item))
            {
                PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to std::string.");
                *sipIsErr = 1;
                break;
            }

            std::string val = "";

          	if (item == Py_None) {
          		return 1;
          	}
          	if (PyUnicode_Check(item))	{
                PyObject* s = PyUnicode_AsEncodedString(item, "UTF-8", "");
                val = std::string(PyString_AS_STRING(s));
                Py_DECREF(s);
                return 1;
          	}
          	if (PyString_Check(item)) {
                val = std::string(PyString_AS_STRING(item));
               	return 1;
          	}
            V->push_back(val);

            Py_DECREF(item);
        }

        Py_DECREF(iterator);

        if (*sipIsErr)
        {
            delete V;
            return 0;
        }
    }

    *sipCppPtr = V;
    return sipGetState(sipTransferObj);
%End
};

// ****************************************************
// Specialization for std::vector<std::vector<std::string>>
// ****************************************************

%MappedType std::vector<std::vector<std::string>>
{
%TypeHeaderCode
#include <vector>
#include <string>
#include <memory>
%End

%ConvertFromTypeCode
  PyObject *outerListPy = nullptr;
  if ((outerListPy = PyList_New((SIP_SSIZE_T)sipCpp->size())) != nullptr) {
    auto outerListIndex = 0;
    for (auto&& innerListCpp : *sipCpp) {
      PyObject* innerListPy = PyList_New((SIP_SSIZE_T)innerListCpp.size());
      if (innerListPy != nullptr) {
        auto innerListIndex = 0;
        for (auto&& itemCpp : innerListCpp) {
          PyObject* itemPy = PyUnicode_DecodeUTF8(itemCpp.c_str(), itemCpp.length(), nullptr);
          if (itemPy != nullptr) {
            PyErr_Clear();
            itemPy = PyString_FromString(itemCpp.c_str());
            if (itemPy != nullptr) {
              PyList_SET_ITEM(innerListPy, innerListIndex, itemPy);
            } else {
              return nullptr;
            }
            ++innerListIndex;
          } else {
            return nullptr;
          }
        }
        PyList_SET_ITEM(outerListPy, outerListIndex, innerListPy);
        ++outerListIndex;
      } else {
        return nullptr;
      }
    }
    return outerListPy;
  } else {
    return nullptr;
  }
%End

%ConvertToTypeCode
  // Check if type is compatible
  if (sipIsErr == nullptr) {
    // Must be any iterable
    PyObject *i = PyObject_GetIter(sipPy);
    bool iterable = (i != nullptr);
    Py_XDECREF(i);
    return iterable;
  } else {
    auto outerListLength = static_cast<int>(PyObject_Size(sipPy));
    auto outerListCpp = std::make_unique<std::vector<std::vector<std::string>>>();
    outerListCpp->reserve(outerListLength);
    if (outerListLength > 0) {
      PyObject *outerListIteratorPy = PyObject_GetIter(sipPy);
      if (outerListIteratorPy != nullptr) {
        PyObject *innerListPy = nullptr;
        while ((innerListPy = PyIter_Next(outerListIteratorPy))) {
          PyObject *innerListIteratorPy = PyObject_GetIter(innerListPy);
          if (innerListIteratorPy != nullptr) {
            auto innerListCpp = std::vector<std::string>();
            auto innerListLength = static_cast<int>(PyObject_Size(innerListPy));
            innerListCpp.reserve(innerListLength);

            PyObject *itemPy = nullptr;
            while ((itemPy = PyIter_Next(innerListIteratorPy))) {
              auto itemString = std::string("");
            	if (itemPy == Py_None) {
            	} else if (PyUnicode_Check(itemPy)) {
                PyObject* s = PyUnicode_AsEncodedString(itemPy, "UTF-8", "");
                itemString = std::string(PyString_AS_STRING(s));
                Py_DECREF(s);
            	} else if (PyString_Check(itemPy)) {
                itemString = std::string(PyString_AS_STRING(itemPy));
            	}
              innerListCpp.emplace_back(std::move(itemString));
            }
            Py_DECREF(innerListIteratorPy);
            Py_DECREF(innerListPy);
            outerListCpp->emplace_back(std::move(innerListCpp));
          } else {
            *sipIsErr = 1;
            PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to iterable.");
            Py_DECREF(innerListIteratorPy);
            Py_DECREF(outerListIteratorPy);
            return 0;
          }
        }
        Py_DECREF(outerListIteratorPy);
      } else {
        *sipIsErr = 1;
        PyErr_Format(PyExc_TypeError, "failed to convert object expected to be not iterable.");
        Py_DECREF(outerListIteratorPy);
        return 0;
      }
    }
    *sipCppPtr = outerListCpp.release();
    return sipGetState(sipTransferObj);
  }
%End
};
