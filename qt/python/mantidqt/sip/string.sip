// Adapted from the PyQt mailing list at
// https://riverbankcomputing.com/pipermail/pyqt/2005-February/009662.html

%MappedType std::string
{
%TypeHeaderCode
#include <string>
%End

%ConvertFromTypeCode
#if PY_VERSION_HEX >= 0x03000000
    return PyUnicode_FromString(sipCpp->c_str());
#else
    return PyString_FromString(sipCpp->c_str());
#endif
%End

%ConvertToTypeCode
   // Allow a Python string (or a unicode string) whenever a string is
   // expected.
   if (sipIsErr == NULL)
    return (SIPBytes_Check(sipPy) || PyUnicode_Check(sipPy));
   if (sipPy == Py_None) {
     *sipCppPtr = new std::string;
     return 1;
   }
   if (PyUnicode_Check(sipPy)) {
    // encode with UTF-8 to bytes object
    PyObject* s = PyUnicode_AsUTF8String(sipPy);
    *sipCppPtr = new std::string(SIPBytes_AS_STRING(s));
    Py_DECREF(s);
    return 1;
   }
   if (SIPBytes_Check(sipPy)) {
    *sipCppPtr = new std::string(SIPBytes_AS_STRING(sipPy));
    return 1;
   }
  return 0;
%End
};
