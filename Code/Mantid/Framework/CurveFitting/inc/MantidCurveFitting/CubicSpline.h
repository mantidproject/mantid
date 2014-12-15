#ifndef MANTID_CURVEFITTING_CubicSpline_H_
#define MANTID_CURVEFITTING_CubicSpline_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BackgroundFunction.h"

#include <boost/scoped_array.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <valarray>

namespace Mantid
{
  namespace CurveFitting
  {
    /**

    A wrapper around GSL functions implementing cubic spline interpolation.
    This function can also calculate derivatives up to order 2 as a by product of the spline.

    @author Samuel Jackson, STFC
    @date 05/07/2013

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport CubicSpline : public BackgroundFunction
    {

    public:
      /// Constructor
      CubicSpline();
      /// Destructor
      ~CubicSpline();

      /// overwrite IFunction base class methods
      std::string name()const{return "CubicSpline";}
      virtual const std::string category() const { return "Background";}
      void function1D(double* out, const double* xValues, const size_t nData)const;
      void derivative1D(double* out, const double* xValues, size_t nData, const size_t order) const;
      void setParameter(size_t i, const double& value, bool explicitlySet=true);
      using ParamFunction::setParameter;

      /// Set a value to attribute attName
      void setAttribute(const std::string& attName,const Attribute& );

      /// Set the value of a data point location to x
      void setXAttribute(const size_t index, double x);

    private:

      /// Minimum number of data points in spline
      const int m_min_points;

      /// Functor to free a GSL objects in a shared pointer
      struct GSLFree
      {
        void operator()(gsl_spline* spline) {gsl_spline_free(spline);}
        void operator()(gsl_interp_accel* acc) {gsl_interp_accel_free(acc);}
      } m_gslFree;

      /// GSL interpolation accelerator object
      boost::shared_ptr<gsl_interp_accel> m_acc;

      /// GSL data structure used to calculate spline
      boost::shared_ptr<gsl_spline> m_spline;

      /// Flag for checking if the spline needs recalculating
      mutable bool m_recalculateSpline;

      /// Reallocate the spline object to use n data points
      void reallocGSLObjects(const int n);

      /// Method to setup the gsl function
      void setupInput(boost::scoped_array<double>& x, boost::scoped_array<double>& y, int n) const;

      /// Calculate the spline
      void calculateSpline(double* out, const double* xValues, const size_t nData) const;

      /// Calculate the derivative
      void calculateDerivative(double* out, const double* xValues,
          const size_t nData, const size_t order) const;

      /// Initialise GSL objects if required
      void initGSLObjects(boost::scoped_array<double>& x, boost::scoped_array<double>& y, int n) const;

      /// Check if an error occurred and throw appropriate message
      void checkGSLError(const int status, const int errorType) const;

      /// Check if an x value falls within the range of the spline
      bool checkXInRange(double x) const;

      /// Evaluate a point on the spline, with basic error handling
      double splineEval(const double x) const;
    };


    typedef boost::shared_ptr<CubicSpline> CubicSpline_sptr;
    typedef const boost::shared_ptr<CubicSpline> CubicSpline_const_sptr;

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CubicSpline_H_*/
