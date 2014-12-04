#ifndef MANTID_CURVEFITTING_BACKTOBACKEXPONENTIAL_H_
#define MANTID_CURVEFITTING_BACKTOBACKEXPONENTIAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Provide BackToBackExponential peak shape function interface to IPeakFunction.
    That is the function:

      I*(A*B/(A+B)/2)*(exp(A/2*(A*S^2+2*(x-X0)))*erfc((A*S^2+(x-X0))/sqrt(2*S^2))+exp(B/2*(B*S^2-2*(x-X0)))*erfc((B*S^2-(x-X0))/sqrt(2*S^2))).

    Function parameters:
    <UL>
    <LI> I - Integrated intensity of peak (default 0.0)</LI>
    <LI> A - exponential constant of rising part of neutron pulse (default 1.0)</LI>
    <LI> B - exponential constant of decaying part of neutron pulse (default 0.05)</LI>
    <LI> X0 - peak position (default 0.0)</LI>
    <LI> S - standard deviation of gaussian part of peakshape function (default 1.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 9/11/2009

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
    class DLLExport BackToBackExponential : public API::IPeakFunction
    {
    public:
      /// Default constructor.
      BackToBackExponential():API::IPeakFunction(){}

      /// overwrite IPeakFunction base class methods
      virtual double centre()const {return getParameter("X0");}
      virtual void setCentre(const double c) {setParameter("X0",c);}
      virtual double height()const;
      virtual void setHeight(const double h) ;
      virtual double fwhm()const;
      virtual void setFwhm(const double w);

      /// overwrite IFunction base class methods
      std::string name()const{return "BackToBackExponential";}
      virtual const std::string category() const { return "Peak";}
      virtual void function1D(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);

    protected:
      /// overwrite IFunction base class method, which declare function parameters
      virtual void init();
      /// Function evaluation method to be implemented in the inherited classes
      virtual void functionLocal(double*, const double*, const size_t )const{}
      /// Derivative evaluation method to be implemented in the inherited classes
      virtual void functionDerivLocal(API::Jacobian*, const double*, const size_t){}
      double expWidth() const;
    };

    typedef boost::shared_ptr<BackToBackExponential> BackToBackExponential_sptr;

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_BACKTOBACKEXPONENTIAL_H_*/
