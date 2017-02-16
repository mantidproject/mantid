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

%ConvertToTypeCode
if (sipIsErr == NULL)
    return PyString_Check(sipPy);

    *sipCppPtr = new std::string(PyString_AsString(sipPy),
PyString_Size(sipPy));

    return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
return PyString_FromStringAndSize(sipCpp->data(),sipCpp->length());
%End

};


