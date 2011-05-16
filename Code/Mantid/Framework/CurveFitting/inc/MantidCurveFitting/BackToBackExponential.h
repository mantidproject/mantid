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

      I*(exp(A/2*(A*S^2+2*(x-X0)))*erfc((A*S^2+(x-X0))/sqrt(2*S^2))+exp(B/2*(B*S^2-2*(x-X0)))*erfc((B*S^2-(x-X0))/sqrt(2*S^2))).

    Function parameters:
    <UL>
    <LI> I - height of peak (default 0.0)</LI>
    <LI> A - exponential constant of rising part of neutron pulse (default 0.0)</LI>
    <LI> B - exponential constant of decaying part of neutron pulse (default 0.0)</LI>
    <LI> X0 - peak position (default 0.0)</LI>
    <LI> S - standard deviation of gaussian part of peakshape function (default 1.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 9/11/2009

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
    class DLLExport BackToBackExponential : public API::IPeakFunction
    {
    public:
      /// Destructor
      virtual ~BackToBackExponential() {};


      /// overwrite IPeakFunction base class methods
      virtual double centre()const {return getParameter("X0");};
      virtual double height()const {return getParameter("I");};  // note height can likely be defined more accurately, here set equal to intensity 
      virtual double width()const {return 2*getParameter("S");};  // can likely be defined more accurately
      virtual void setCentre(const double c) {setParameter("X0",c);};
      virtual void setHeight(const double h) {setParameter("I",h);};
      virtual void setWidth(const double w) {setParameter("S",w/2.0);};


      /// overwrite IFunction base class methods
      std::string name()const{return "BackToBackExponential";}
      virtual void function(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDeriv(API::Jacobian* out, const double* xValues, const size_t nData);

    protected:
      /// overwrite IFunction base class method, which declare function parameters
      virtual void init();
      /// Function evaluation method to be implemented in the inherited classes
      virtual void functionLocal(double* out, const double* xValues, const size_t nData)const
      {
        UNUSED_ARG(out); UNUSED_ARG(xValues); UNUSED_ARG(nData);
      }

      /// Derivative evaluation method to be implemented in the inherited classes
      virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData)
      {
        UNUSED_ARG(out); UNUSED_ARG(xValues); UNUSED_ARG(nData);
      }

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_BACKTOBACKEXPONENTIAL_H_*/
