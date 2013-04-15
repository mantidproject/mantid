#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"

#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"

#include <boost/python/class.hpp>

//-----------------------------------------------------------------------------
// IFunction1D definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {
    using Environment::CallMethod2;
    using namespace boost::python;

    /**
     * Construct the "wrapper" and stores the reference to the PyObject
     * * @param self A reference to the calling Python object
     */
    IFunction1DAdapter::IFunction1DAdapter(PyObject* self)
      : API::ParamFunction(), API::IFunction1D(), IFunctionAdapter(self)
    {
    }

    /**
     * Translates between the C++ signature & the Python signature
     * @param out The 1D data array of size nData that stores the output values
     * @param xValues The input X values
     * @param nData The size of the two arrays
     */
    void IFunction1DAdapter::function1D(double* out, const double* xValues, const size_t nData) const
    {
      Py_intptr_t dims[1] = { static_cast<Py_intptr_t>(nData) } ;
      object xvals = object(handle<>(Converters::WrapReadOnly::apply<double>::createFromArray(xValues, 1,dims)));
      object outnp = object(handle<>(Converters::WrapReadWrite::apply<double>::createFromArray(out, 1,dims)));
      function1D(xvals, outnp);
    }

    /**
     * Python-type signature version of above
     * @param xvals The input X values in read-only numpy array
     * @param out A read/write numpy array of doubles to store the results
     */
    void IFunction1DAdapter::function1D(const boost::python::object & xvals, boost::python::object & out) const
    {
      CallMethod2<void,object,object>::dispatchWithException(getSelf(), "function1D", xvals, out);
    }
  }
}
