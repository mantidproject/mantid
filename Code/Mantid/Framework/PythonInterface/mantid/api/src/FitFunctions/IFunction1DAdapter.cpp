#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"

#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"

#include <boost/python/call_method.hpp>
#include <boost/python/class.hpp>

//-----------------------------------------------------------------------------
// IFunction1D definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {
    using namespace boost::python;


    /**
     * Construct the "wrapper" and stores the reference to the PyObject
     * * @param self A reference to the calling Python object
     */
    IFunction1DAdapter::IFunction1DAdapter(PyObject* self)
      : IFunction1D(), m_self(self)
    {
    }

    /**
     * Returns the class name of the function. This cannot be overridden in Python.
     */
    std::string IFunction1DAdapter::name() const
    {
      return std::string(getSelf()->ob_type->tp_name);
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
      function1D(outnp, xvals);
    }

    /**
     * Python-type signature version of above
     * @param out A read/write numpy array of doubles to store the results
     * @param xvals The input X values in read-only numpy array
     */
    void IFunction1DAdapter::function1D(boost::python::object & out, const boost::python::object & xvals) const
    {
      boost::python::call_method<void,object,object>(getSelf(), "function1D", out, xvals);
    }


  }
}
