#ifndef MANTID_CURVEFITTING_GAUSSIAN_H_
#define MANTID_CURVEFITTING_GAUSSIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include <cmath>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Provide gaussian peak shape function interface to IPeakFunction.
    I.e. the function: Height*exp(-0.5*((x-PeakCentre)/Sigma)^2).

    This function actually performs the fitting on 1/Sigma^2 rather than Sigma
    for stability reasons.

    Gauassian parameters:
    <UL>
    <LI> Height - height of peak (default 0.0)</LI>
    <LI> PeakCentre - centre of peak (default 0.0)</LI>
    <LI> Sigma - standard deviation (default 0.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 19/10/2009

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport Gaussian : public API::IPeakFunction
    {
    public:
      /// Destructor
      virtual ~Gaussian() {};


      /// overwrite IPeakFunction base class methods
      virtual double centre()const {return getParameter("PeakCentre");}
      virtual double height()const {return getParameter("Height");}
      virtual double width()const {return 2.0*sqrt(2.0*std::log(2.0))*getParameter("Sigma");}
      virtual void setCentre(const double c) {setParameter("PeakCentre",c);}
      virtual void setHeight(const double h) {setParameter("Height",h);}
      virtual void setWidth(const double w) {setParameter("Sigma",w/(2.0*sqrt(2.0*std::log(2.0))));}


      /// overwrite IFunction base class methods
      std::string name()const{return "Gaussian";}
      virtual void calJacobianForCovariance(API::Jacobian* out, const double* xValues, const int& nData);
      virtual void setActiveParameter(int i,double value);
      virtual double activeParameter(int i)const;

    protected:
      virtual void functionLocal(double* out, const double* xValues, const int& nData)const;
      virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const int& nData);
      /// overwrite IFunction base class method, which declare function parameters
      virtual void init();
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN_H_*/
