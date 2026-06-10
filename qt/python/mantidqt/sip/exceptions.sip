// ----------------------------------------------------------------------------
// Exceptions
// ----------------------------------------------------------------------------

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

// Marked /Default/ so every wrapped call converts C++ exceptions to Python without a
// per-function throw() specifier (the throw() exception specification is deprecated in
// sip v6 and removed in v7). Opt a function out with a trailing `noexcept`.
%Exception std::exception(SIP_Exception) /PyName=BaseCppError, Default/
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
