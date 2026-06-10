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

// Marked /Default/ so functions without an explicit exception specifier still convert any
// C++ exception to Python (as BaseCppError). The deprecated throw() specifier is therefore
// only kept where a specific Python type is required - e.g. throw(std::invalid_argument)
// maps to ValueError - because sip v6 offers no non-deprecated way to express that per
// function; such mappings collapse to BaseCppError if the throw() is dropped.
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
