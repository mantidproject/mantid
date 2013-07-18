/*WIKI*

This function interpolates between a set of data points.

First and second derivatives can be calculated by using the derivative1D function.

CubicSpline function has two sets of attributes. First is 'n' which has
integer type and sets the number of interpolation points.
The parameter names have the form 'yi' where 'y' is letter 'y' and 'i' is the
parameter's index starting from 0 and have the type double.
Likewise, the attribute names have the form 'xi'.

 *WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CubicSpline.h"
#include "MantidAPI/FunctionFactory.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace Mantid
{
  namespace CurveFitting
  {
    using namespace Kernel;
    using namespace API;

    DECLARE_FUNCTION(CubicSpline)

    const int CubicSpline::M_MIN_POINTS = gsl_interp_type_min_size (gsl_interp_cspline);

    CubicSpline::CubicSpline() :
        m_acc(gsl_interp_accel_alloc(), m_gslFree),
        m_spline(gsl_spline_alloc(gsl_interp_cspline, M_MIN_POINTS), m_gslFree),
        m_recalculateSpline(true)
    {
      //setup class with a default set of attributes
      declareAttribute("n", Attribute(M_MIN_POINTS));

      declareAttribute("x0", Attribute(0.0));
      declareAttribute("x1", Attribute(1.0));
      declareAttribute("x2", Attribute(2.0));

      declareParameter("y0", 0);
      declareParameter("y1", 0);
      declareParameter("y2", 0);
    };

    void CubicSpline::function1D(double* out, const double* xValues, const size_t nData) const
    {
      //check if spline needs recalculating
      int n = getAttribute("n").asInt();

      boost::scoped_array<double> x(new double[n]);
      boost::scoped_array<double> y(new double[n]);

      //setup the reference points and calculate
      if(m_recalculateSpline) setupInput(x,y,n);

      calculateSpline(out, xValues, nData);

      m_recalculateSpline = false;
    }

    void CubicSpline::setupInput(boost::scoped_array<double>& x,
        boost::scoped_array<double>& y, int n) const
    {
      //Populate data points from the input attributes and parameters
      bool xSortFlag = false;

      for (int i = 0; i < n; ++i)
      {
        std::string num = boost::lexical_cast<std::string>(i);

        std::string xName = "x" + num;
        std::string yName = "y" + num;

        x[i] = getAttribute(xName).asDouble();

        //if x[i] is out of order with its neighbours
        if(i>1 && i < n &&
            (x[i-1] < x[i-2] || x[i-1] > x[i]))
        {
          xSortFlag = true;
        }

        y[i] = getParameter(yName);
      }

      //sort the data points if necessary
      if(xSortFlag)
      {
        g_log.warning() << "Spline x parameters are not in ascending order. Values will be sorted." << std::endl;
        std::sort(x.get(), x.get()+n);
      }

      //pass values to GSL objects
      initGSLObjects(x,y,n);
    }

    void CubicSpline::derivative1D(double* out, const double* xValues, size_t nData, const size_t order) const
    {
      int n = getAttribute("n").asInt();

      boost::scoped_array<double> x(new double[n]);
      boost::scoped_array<double> y(new double[n]);

      //setup the reference points and calculate
      if(m_recalculateSpline) setupInput(x,y,n);
      calculateDerivative(out,xValues,nData,order);

      m_recalculateSpline = false;
    }

    bool CubicSpline::checkXInRange(double x) const
    {
      return (x >= m_spline->interp->xmin
          || x <= m_spline->interp->xmax);
    }

    void CubicSpline::calculateSpline(double* out, const double* xValues, const size_t nData) const
    {
      //calculate spline for given input set
      double y(0);
      bool outOfRange(false);
      for (size_t i = 0; i < nData; ++i)
      {
        if(checkXInRange(xValues[i]))
        {
          y = gsl_spline_eval(m_spline.get(), xValues[i], m_acc.get());
          int errorCode = gsl_spline_eval_e(m_spline.get(), xValues[i], m_acc.get(), &y);

          //check if GSL function returned an error
          checkGSLError(errorCode, GSL_EDOM);

          out[i] = y;
        }
        else
        {
          outOfRange = true;
          y = 0;
        }
      }

      if(outOfRange)
      {
        g_log.warning() << "Some x values where out of range and will not be calculated." << std::endl;
      }
    }

    void CubicSpline::calculateDerivative(double* out, const double* xValues,
        const size_t nData, const size_t order) const
    {
      double xDeriv = 0;
      int errorCode = 0;
      bool outOfRange(false);

      //throw error if the order is not the 1st or 2nd derivative
      if(order > 2 || order < 1) throw std::invalid_argument(
          "CubicSpline: order of derivative must be either 1 or 2");

      for(size_t i = 0; i < nData; ++i)
      {
        if(checkXInRange(xValues[i]))
        {
          //choose the order of the derivative
          if(order == 1)
          {
            xDeriv = gsl_spline_eval_deriv(m_spline.get(),xValues[i],m_acc.get());
            errorCode = gsl_spline_eval_deriv_e (m_spline.get(),xValues[i],m_acc.get(),&xDeriv);
          }
          else if (order == 2)
          {
            xDeriv = gsl_spline_eval_deriv2(m_spline.get(),xValues[i],m_acc.get());
            errorCode = gsl_spline_eval_deriv2_e (m_spline.get(),xValues[i],m_acc.get(),&xDeriv);
          }
        }
        else
        {
          outOfRange = true;
          xDeriv = 0;
        }

        //check GSL functions didn't return an error
        checkGSLError(errorCode, GSL_EDOM);

        //record the value
        out[i] = xDeriv;
      }

      if(outOfRange)
      {
        g_log.warning() << "Some x values where out of range and will not be calculated." << std::endl;
      }
    }

    void CubicSpline::setParameter(size_t i, const double& value, bool explicitlySet)
    {
      //Call parent setParameter implementation
      ParamFunction::setParameter(i, value, explicitlySet);

      //recalculate if necessary
      m_recalculateSpline = true;
    }

    void CubicSpline::setAttribute(const std::string& attName, const API::IFunction::Attribute& att)
    {

      if (attName == "n")
      {
        //get the new and old number of data points
        int n = att.asInt();
        int oldN = getAttribute("n").asInt();

        //check that the number of data points is in a valid range
        if (n > oldN)
        {
          //get the name of the last x data point
          std::string oldXName = "x" + boost::lexical_cast<std::string>(oldN - 1);
          double oldX = getAttribute(oldXName).asDouble();

          //reallocate gsl object to new size
          reallocGSLObjects(n);

          //create blank a number of new blank parameters and attributes
          for (int i = oldN; i < n; ++i)
          {
            std::string num = boost::lexical_cast<std::string>(i);

            std::string newXName = "x" + num;
            std::string newYName = "y" + num;

            declareAttribute(newXName, Attribute(oldX + static_cast<double>(i - oldN + 1)));
            declareParameter(newYName, 0);
          }

          //flag that the spline + derivatives will now need to be recalculated
          m_recalculateSpline = true;
        }
        else if (n < oldN)
        {
          throw std::invalid_argument("Cubic Spline: Can't decrease the number of attributes");
        }
      }

      storeAttributeValue(attName, att);
    }

    void CubicSpline::setXAttribute(const size_t index, double x)
    {
      size_t n = static_cast<size_t>(getAttribute("n").asInt());

      //check that setting the x attribute is within our range
      if (index < n)
      {
        std::string xName = "x" + boost::lexical_cast<std::string>(index);
        setAttributeValue(xName, x);

        //attributes updated, flag for recalculation
        m_recalculateSpline = true;
      }
      else
      {
        throw std::range_error("Cubic Spline: x index out of range.");
      }
    }

    void CubicSpline::checkGSLError(const int status, const int errorType) const
    {
      //check GSL functions didn't return an error
      if(status == errorType)
      {
        m_recalculateSpline = true;

        std::string message("CubicSpline: ");
        message.append(gsl_strerror(errorType));

        throw std::runtime_error(message);
      }
    }

    void CubicSpline::initGSLObjects(boost::scoped_array<double>& x, boost::scoped_array<double>& y, int n) const
    {
      int status = gsl_spline_init(m_spline.get(), x.get(), y.get(), n);
      checkGSLError(status, GSL_EINVAL);
    }

    void CubicSpline::reallocGSLObjects(const int n)
    {
      m_spline.reset(gsl_spline_alloc(gsl_interp_cspline, n),m_gslFree);
      gsl_interp_accel_reset (m_acc.get());
    }

    CubicSpline::~CubicSpline()
    {

    }
  } // namespace CurveFitting
} // namespace Mantid
