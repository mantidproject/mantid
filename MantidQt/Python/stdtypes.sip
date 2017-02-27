/***************************************************************************/
/**
 This file is a SIP file for binding types used in std library.
 This is not a stand-alone SIP file, it must be "%Include"d into
 another
*/
/***************************************************************************/


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


%MappedType std::string
{
%TypeHeaderCode
#include <string>
%End

%ConvertFromTypeCode
    // convert an std::string to a Python (unicode) string
    PyObject* newstring;
    newstring = PyUnicode_DecodeUTF8(sipCpp->c_str(), sipCpp->length(), NULL);
    if(newstring == NULL) {
        PyErr_Clear();
        newstring = PyUnicode_FromString(sipCpp->c_str());
    }
    return newstring;
%End

%ConvertToTypeCode
     // Allow a Python string (or a unicode string) whenever a string is
     // expected.
    // If argument is a Unicode string, just decode it to UTF-8
    // If argument is a Python string, assume it's UTF-8
     if (sipIsErr == NULL)
#if PY_MAJOR_VERSION < 3
        return (PyString_Check(sipPy) || PyUnicode_Check(sipPy));
#else
        return PyUnicode_Check(sipPy);
#endif
     if (sipPy == Py_None) {
        *sipCppPtr = new std::string;
         return 1;
     }
     if (PyUnicode_Check(sipPy)) {
        PyObject* s = PyUnicode_AsEncodedString(sipPy, "UTF-8", "");
        *sipCppPtr = new std::string(PyUnicode_AS_DATA(s));
        Py_DECREF(s);
        return 1;
     }
#if PY_MAJOR_VERSION < 3
     if (PyString_Check(sipPy)) {
        *sipCppPtr = new std::string(PyString_AS_STRING(sipPy));
        return 1;
     }
#endif

     return 0;
%End
};

template<TYPE>
%MappedType std::set<TYPE>
{
%TypeHeaderCode
#include <set>
%End

%ConvertFromTypeCode
    PyObject *l;

        // Create the Python list of the correct length.
        if ((l = PyList_New(sipCpp->size())) == NULL) {
                return NULL;
        }

        // Go through each element in the C++ instance and convert it to a
        // wrapped P2d.
        int i = 0;
        const sipMappedType* mappedType = sipFindMappedType("TYPE");

    for (std::set<TYPE>::const_iterator it = sipCpp->begin();
        it != sipCpp->end(); ++it) {

        TYPE *cpp = new TYPE(*it);
        PyObject *pobj;

		pobj = sipConvertFromMappedType(cpp, mappedType, Py_None);

        // Add the wrapper to the list.
        PyList_SET_ITEM(l, i, pobj);
        i++;
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

        std::set<TYPE> *V = new std::set<TYPE>();
        const sipMappedType* mappedType = sipFindMappedType("TYPE");

        while ((item = PyIter_Next(iterator)))
        {
                if (!sipCanConvertToMappedType(item, mappedType, SIP_NOT_NONE)) {
                        PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to TYPE");
                        *sipIsErr = 1;
                        break;
                }

                int state;
                TYPE *p =
                        reinterpret_cast<TYPE *>(sipConvertToMappedType(item, mappedType,
                                Py_None, SIP_NOT_NONE, &state, sipIsErr));

                if (!*sipIsErr) {
                        V->insert(*p);
                }

                sipReleaseMappedType(p, mappedType, state);
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