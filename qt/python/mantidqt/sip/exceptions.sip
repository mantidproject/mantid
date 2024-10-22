// ----------------------------------------------------------------------------
// Exceptions
// ----------------------------------------------------------------------------

%Exception std::exception(SIP_Exception) /PyName=BaseCppError/
{
%TypeHeaderCode
#include <exception>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(PyExc_Exception, detail);
    SIP_UNBLOCK_THREADS
%End
};

%Exception std::runtime_error(SIP_Exception) /PyName=RuntimeError/
{
%TypeHeaderCode
#include <stdexcept>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(PyExc_RuntimeError, detail);
    SIP_UNBLOCK_THREADS
%End
};

%Exception std::invalid_argument(SIP_Exception) /PyName=ValueError/
{
%TypeHeaderCode
#include <stdexcept>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(PyExc_ValueError, detail);
    SIP_UNBLOCK_THREADS
%End
};
