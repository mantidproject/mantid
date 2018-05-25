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
     PyObject* newstring;
     newstring = PyUnicode_DecodeUTF8(sipCpp->c_str(), sipCpp->length(), NULL);
     if(newstring == NULL) {
         PyErr_Clear();
         newstring = FROM_CSTRING(sipCpp->c_str());
     }
     return newstring;
%End

%ConvertToTypeCode
	// Allow a Python string (or a unicode string) whenever a string is
	// expected.
     // If argument is a Unicode string, just decode it to UTF-8
     // If argument is a Python string, assume it's UTF-8
	if (sipIsErr == NULL)
		return (STR_CHECK(sipPy) || PyUnicode_Check(sipPy));
	if (sipPy == Py_None) {
		*sipCppPtr = new std::string;
		return 1;
	}
	if (PyUnicode_Check(sipPy))	{
         PyObject* s = PyUnicode_AsEncodedString(sipPy, "UTF-8", "");
         *sipCppPtr = new std::string(TO_CSTRING(s));
         Py_DECREF(s);
         return 1;
	}
	if (STR_CHECK(sipPy)) {
         *sipCppPtr = new std::string(TO_CSTRING(sipPy));
     	return 1;
	}
  return 0;
%End
};
