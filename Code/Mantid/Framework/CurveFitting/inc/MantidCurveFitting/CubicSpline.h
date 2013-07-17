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
    Implements cubic spline Interpolation.

    Attributes: int n - number of interpolation points
    Parameters: n


    @author Samuel Jackson, STFC
    @date 05/07/2013

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
      /// GSL interpolation accelerator object
      gsl_interp_accel* m_acc;

      /// GSL data structure used to calculate spline
      gsl_spline* m_spline;

      /// GSL data structure used to calculate derivatives
      gsl_interp* m_interp;

      /// Flag for checking if the spline needs recalculating
      mutable bool m_recalculateSpline;

      /// Minimum number of data points in spline
      static const int M_MIN_POINTS;

      /// Reallocate the spline object to use n data points
      void reallocGSLObjects(const int n);

      /// Method to setup the gsl function
      void setupInput(boost::scoped_array<double>& x, boost::scoped_array<double>& y, int n) const;

      /// Calculate the spline
      void calculateSpline(double* out, const double* xValues, const size_t nData) const;

      /// Calculate the derivative
      void calculateDerivative(const boost::scoped_array<double>& x, const boost::scoped_array<double>& y,
          double* out, const double* xValues, const size_t nData, const size_t order) const;

      /// Initialise GSL objects if required
      void initGSLObjects(boost::scoped_array<double>& x, boost::scoped_array<double>& y, int n) const;

      /// Check if an error occurred and throw appropriate message
      void checkGSLError(const int status, const int errorType) const;

      /// Check if an x value falls within the range of the spline
      bool checkXInRange(double x) const;
    };


    typedef boost::shared_ptr<CubicSpline> CubicSpline_sptr;

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CubicSpline_H_*/
