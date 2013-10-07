#ifndef MANTID_CURVEFITTING_LORENTZIAN_H_
#define MANTID_CURVEFITTING_LORENTZIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Provide lorentzian peak shape function interface to IPeakFunction.
    I.e. the function: <math>\frac{A}{\pi}( \Gamma/((x-PeakCentre)^2+HWHM^2) ).


    Lorentzian parameters:
    <UL>
    <LI> Amplitude - Maximum height of peak at x=PeakCentre(default 1.0)</LI>
    <LI> PeakCentre - centre of peak (default 0.0)</LI>
    <LI> HWHM - half-width half-maximum (default 0.0)</LI>
    </UL>

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
    class DLLExport Lorentzian : public API::IPeakFunction
    {
    public:
      /// Destructor
      virtual ~Lorentzian() {};

      /// overwrite IPeakFunction base class methods
      virtual double centre()const {return getParameter("PeakCentre");}
      virtual double height()const {return getParameter("Amplitude");}
      virtual double fwhm()const {return 2*getParameter("HWHM");}
      virtual void setCentre(const double c) {setParameter("PeakCentre",c);}
      virtual void setHeight(const double h) {setParameter("Amplitude",h);}
      virtual void setFwhm(const double w) {setParameter("HWHM",w/2.0);}

      /// overwrite IFunction base class methods
      std::string name()const{return "Lorentzian";}
      virtual const std::string category() const { return "Peak";}
    protected:
      virtual void functionLocal(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData);
      /// overwrite IFunction base class method, which declare function parameters
      virtual void init();

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LORENTZIAN_H_*/
