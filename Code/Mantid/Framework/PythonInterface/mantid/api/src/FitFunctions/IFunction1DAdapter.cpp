#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"

#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"

#include <boost/python/class.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

//-----------------------------------------------------------------------------
// IFunction1D definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {
    using Environment::CallMethod1;
    using namespace boost::python;

    /**
     * Construct the "wrapper" and stores the reference to the PyObject
     * @param self A reference to the calling Python object
     */
    IFunction1DAdapter::IFunction1DAdapter(PyObject* self)
      : API::ParamFunction(), API::IFunction1D(), IFunctionAdapter(self), m_derivOveridden(false)
    {
      m_derivOveridden = Environment::typeHasAttribute(self, "functionDeriv1D");
    }

    /**
     * Translates between the C++ signature & the Python signature called by Fit
     * @param out The 1D data array of size nData that stores the output values
     * @param xValues The input X values
     * @param nData The size of the two arrays
     */
    void IFunction1DAdapter::function1D(double* out, const double* xValues, const size_t nData) const
    {
      using namespace Converters;
      // GIL must be held while numpy wrappers are destroyed as they access Python
      // state information
      Environment::GlobalInterpreterLock gil;

      Py_intptr_t dims[1] = { static_cast<Py_intptr_t>(nData) };
      PyObject *xvals = WrapReadOnly::apply<double>::createFromArray(xValues, 1,dims);

      // Deliberately avoids using the CallMethod wrappers. They lock the GIL again and
      // will check for each function call whether the wrapped method exists. It also avoid unnecessary construction of
      // boost::python::objects whn using boost::python::call_method

      PyObject *result = PyEval_CallMethod(getSelf(), "function1D", "(O)", xvals);
      if(PyErr_Occurred()) Environment::throwRuntimeError(true);

      PyArrayObject *nparray = (PyArrayObject *)(result);
      if(PyArray_TYPE(nparray) == NPY_DOUBLE) // dtype matches so use memcpy for speed
      {
        std::memcpy(static_cast<void*>(out), PyArray_DATA(nparray), nData*sizeof(npy_double));
      }
      else
      {
        PyArray_Descr *dtype=PyArray_DESCR(nparray);
        PyObject *name = PyList_GetItem(dtype->names, 0);
        std::ostringstream os;
        os << "Unsupported numpy data type: '" << PyString_AsString(name) << "'. Currently only numpy.float64 is supported";
        throw std::runtime_error(os.str());
      }
    }

    /**
     * Python-type signature version of above to be called directly from Python
     * @param xvals The input X values in read-only numpy array
     */
    boost::python::object IFunction1DAdapter::function1D(const boost::python::object & xvals) const
    {
      return CallMethod1<object,object>::dispatchWithException(getSelf(), "function1D", xvals);
    }

    /**
     * If a Python override exists then call that, otherwise call the base class method
     * @param out The Jacobian matrix storing the partial derivatives of the function w.r.t to the parameters
     * @param xValues The input X values
     * @param nData The size of the two arrays
     */
    void IFunction1DAdapter::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
    {
      if(m_derivOveridden)
      {
        using namespace Converters;
        // GIL must be held while numpy wrappers are destroyed as they access Python
        // state information
        Environment::GlobalInterpreterLock gil;

        Py_intptr_t dims[1] = { static_cast<Py_intptr_t>(nData) } ;
        PyObject *xvals = WrapReadOnly::apply<double>::createFromArray(xValues, 1,dims);
        PyObject *jacobian = boost::python::to_python_value<API::Jacobian*>()(out);

        // Deliberately avoids using the CallMethod wrappers. They lock the GIL again and
        // will check for each function call whether the wrapped method exists. It also avoid unnecessary construction of
        // boost::python::objects when using boost::python::call_method
        PyEval_CallMethod(getSelf(), "functionDeriv1D", "(OO)", xvals,jacobian);
        if(PyErr_Occurred()) Environment::throwRuntimeError(true);
      }
      else
      {
        IFunction1D::functionDeriv1D(out,xValues,nData);
      }
    }

  }
}
