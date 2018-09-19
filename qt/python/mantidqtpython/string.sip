// Acquired from the PyQt mailing list at
// https://riverbankcomputing.com/pipermail/pyqt/2005-February/009662.html
// on 9th April 2018.

%MappedType std::string
{
%TypeHeaderCode
#include <string>
%End

%ConvertFromTypeCode
   // convert an std::string to a Python (unicode) string
   PyObject* newstring = PyUnicode_DecodeUTF8(sipCpp->c_str(), sipCpp->length(), NULL);
   if(newstring == NULL) {
     PyErr_Clear();
     newstring = FROM_CSTRING(sipCpp->c_str());
   }
   return newstring;
%End

%ConvertToTypeCode
   // Allow a Python string (or a unicode string) whenever a string is
   // expected.
   if (sipIsErr == NULL)
    return (BYTES_CHECK(sipPy) || STR_CHECK(sipPy));
   if (sipPy == Py_None) {
     *sipCppPtr = new std::string;
     return 1;
   }
   if (PyUnicode_Check(sipPy)) {
    // encode with UTF-8 to bytes object
    PyObject* s = PyUnicode_AsUTF8String(sipPy);
    *sipCppPtr = new std::string(PYBYTES_TO_CSTRING(s));
    Py_DECREF(s);
    return 1;
   }
   if (BYTES_CHECK(sipPy)) {
    *sipCppPtr = new std::string(PYBYTES_TO_CSTRING(sipPy));
    return 1;
   }
  return 0;
%End
};
