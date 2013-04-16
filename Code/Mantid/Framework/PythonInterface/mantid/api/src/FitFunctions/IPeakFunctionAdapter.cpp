#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"

#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

#include <boost/python/class.hpp>

//-----------------------------------------------------------------------------
// IPeakFunction definition
//-----------------------------------------------------------------------------
namespace Mantid
{
  namespace PythonInterface
  {
    using Environment::CallMethod0;
    using Environment::CallMethod1;
    using Environment::CallMethod2;
    using namespace boost::python;

    /**
     * Construct the "wrapper" and stores the reference to the PyObject
     * * @param self A reference to the calling Python object
     */
    IPeakFunctionAdapter::IPeakFunctionAdapter(PyObject* self)
      : API::IPeakFunction(), IFunctionAdapter(self), IFunction1DAdapter(self)
    {
    }

    /**
     */
    double IPeakFunctionAdapter::centre() const
    {
      return CallMethod0<double>::dispatchWithException(getSelf(), "centre");
    }

    /**
     */
    double IPeakFunctionAdapter::height() const
    {
      return CallMethod0<double>::dispatchWithException(getSelf(), "height");
    }

    /**
     * Called when the centre of the peak has been updated outside of the function
     * @param c The centre of the peak
     */
    void IPeakFunctionAdapter::setCentre(const double c)
    {
      CallMethod1<void,double>::dispatchWithException(getSelf(), "setCentre", c);
    }

    /**
     * Called when the height of the peak has been updated outside of the function
     * @param h The new height of the peak
     */
    void IPeakFunctionAdapter::setHeight(const double h)
    {
      CallMethod1<void,double>::dispatchWithException(getSelf(), "setHeight", h);
    }


    /// Calls Python fwhm method
    double IPeakFunctionAdapter::fwhm() const
    {
      return CallMethod0<double>::dispatchWithException(getSelf(), "fwhm");
    }

    /**
     * Called when the width of the peak has been updated outside of the function
     * @param w The new width of the peak. The function should update its parameters such that fwhm=w
     */
    void IPeakFunctionAdapter::setFwhm(const double w)
    {
      return CallMethod1<void,double>::dispatchWithException(getSelf(), "setFwhm", w);
    }


    /**
     * Translates between the C++ signature & the Python signature
     * @param out The 1D data array of size nData that stores the output values
     * @param xValues The input X values
     * @param nData The size of the two arrays
     */
    void IPeakFunctionAdapter::functionLocal(double* out, const double* xValues, const size_t nData) const
    {
      Py_intptr_t dims[1] = { static_cast<Py_intptr_t>(nData) } ;
      object xvals = object(handle<>(Converters::WrapReadOnly::apply<double>::createFromArray(xValues, 1,dims)));
      object outnp = object(handle<>(Converters::WrapReadWrite::apply<double>::createFromArray(out, 1,dims)));
      functionLocal(xvals, outnp);
    }

    /**
     * Python-type signature version of above
     * @param xvals The input X values in read-only numpy array
     * @param out A read/write numpy array of doubles to store the results
     */
    void IPeakFunctionAdapter::functionLocal(const boost::python::object & xvals, boost::python::object & out) const
    {
      CallMethod2<void,object,object>::dispatchWithException(getSelf(), "functionLocal", xvals, out);
    }

    /**
     * Translates between the C++ signature & the Python signature
     * @param out The Jacobian matrix storing the partial derivatives of the function w.r.t to the parameters
     * @param xValues The input X values
     * @param nData The size of the two arrays
     */
    void IPeakFunctionAdapter::functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData)
    {
      Py_intptr_t dims[1] = { static_cast<Py_intptr_t>(nData) } ;
      object xvals = object(handle<>(Converters::WrapReadOnly::apply<double>::createFromArray(xValues, 1,dims)));
      functionDerivLocal(xvals, out);
    }

    /**
     * Python-type signature version of above
     * @param xvals The input X values in read-only numpy array
     *  @param out The Jacobian matrix storing the partial derivatives of the function w.r.t to the parameters
     */
    void IPeakFunctionAdapter::functionDerivLocal(const boost::python::object & xvals, API::Jacobian* out)
    {
      CallMethod2<void,object,API::Jacobian*>::dispatchWithException(getSelf(), "functionDerivLocal", xvals, out);
    }

  }
}
