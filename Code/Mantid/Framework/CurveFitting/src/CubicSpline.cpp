/*WIKI*

 TODO: Add wiki description

 *WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CubicSpline.h"
#include "MantidAPI/FunctionFactory.h"

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

    const int CubicSpline::M_MIN_POINTS = 3;

    CubicSpline::CubicSpline()
      : m_acc(gsl_interp_accel_alloc()),
        m_spline(gsl_spline_alloc(gsl_interp_cspline, M_MIN_POINTS))
    {
      //set the number of attributes to the default
      declareAttribute("n", Attribute(M_MIN_POINTS));

      //init some default x values
      declareAttribute("x0", Attribute(0.0));
      declareAttribute("x1", Attribute(1.0));
      declareAttribute("x2", Attribute(2.0));

      //declare corresponding y values with value 0
      declareParameter("y0", 0);
      declareParameter("y1", 0);
      declareParameter("y2", 0);
    };

    void CubicSpline::function1D(double* out, const double* xValues, const size_t nData) const
    {
      //get the number of parameters constraining the spline
      int n = getAttribute("n").asInt();

      //Vectors to store the parameters/attributes
      std::vector<double> x(n);
      std::vector<double> y(n);

      //Get the cached attributes/parameters
      for (int i = 0; i < n; ++i)
      {
        std::string num = boost::lexical_cast<std::string>(i);
        std::string xName = "x" + num;
        std::string yName = "y" + num;

        x[i] = getAttribute(xName).asDouble();
        y[i] = getParameter(yName);
      }

      //Calculate y from spline for each desired point
      gsl_spline_init(m_spline, x.data(), y.data(), n);
      for (size_t i = 0; i < nData; ++i)
      {
        out[i] = gsl_spline_eval(m_spline, xValues[i], m_acc);
      }
    }

    void CubicSpline::setAttribute(const std::string& attName, const API::IFunction::Attribute& att)
    {
      //set the number of data points that define the spline
      if (attName == "n")
      {
        //get current and new number of data points
        int n = att.asInt();
        int oldN = getAttribute("n").asInt();

        //check if the new n is valid
        if (n > oldN)
        {
          //get the current highest x attribute
          std::string oldXName = "x" + boost::lexical_cast<std::string>(oldN-1);
          double oldX = getAttribute(oldXName).asDouble();

          //resize the spline object
          realloc_spline(n);

          //create and initialise new attributes + parameters
          for (int i = oldN; i < n; ++i)
          {
            std::string num = boost::lexical_cast<std::string>(i);

            std::string newXName = "x" + num;
            std::string newYName = "y" + num;

            declareAttribute(newXName, Attribute(oldX + static_cast<double>(i - oldN+1)));
            declareParameter(newYName, 0);
          }
        }
        else if(n < oldN)
        {
          throw std::invalid_argument("Cubic Spline: Can't decrease the number of attributes");
        }
      }

      storeAttributeValue(attName, att);
    }

    void CubicSpline::setXAttribute(const size_t index, double x)
    {
      //get existing value of n
      size_t n = static_cast<size_t>(getAttribute("n").asInt());

      //check indexed value in within range
      if(index < n)
      {
        //set the new value of the attribute
        std::string xName = "x" + boost::lexical_cast<std::string>(index);
        setAttributeValue(xName, x);
      }
      else
      {
        throw std::range_error("Cubic Spline: x index out of range.");
      }
    }

    void CubicSpline::realloc_spline(const int n)
    {
      gsl_spline_free(m_spline);
      m_spline = gsl_spline_alloc(gsl_interp_cspline, n);
    }

    CubicSpline::~CubicSpline()
    {
      gsl_spline_free(m_spline);
      gsl_interp_accel_free(m_acc);
    }

  } // namespace CurveFitting
} // namespace Mantid
