#ifndef MANTID_CURVEFITTING_Muon_ExpDecayOscTest_H_
#define MANTID_CURVEFITTING_Muon_ExpDecayOscTest_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
//#include "MantidAPI/IFunctionWithLocation.h"
namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Provide lorentzian peak shape function interface to IPeakFunction.
    I.e. the function: Height*( HWHM^2/((x-PeakCentre)^2+HWHM^2) ).


    LorentzianTest parameters:
    <UL>
    <LI> Height - height of peak (default 0.0)</LI>
    <LI> PeakCentre - centre of peak (default 0.0)</LI>
    <LI> HWHM - half-width half-maximum (default 1.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 27/10/2009

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
    class DLLExport Muon_ExpDecayOscTest : public API::IPeakFunction
    {
    public:
      /// Destructor
      virtual ~Muon_ExpDecayOscTest() {};


      /// overwrite IPeakFunction base class methods

      virtual double centre()const {return getParameter("lambda");}
      virtual double height()const {return getParameter("A");}
      virtual double width()const {return 1/getParameter("frequency");}
      virtual void setCentre(const double c) {setParameter("lambda",c);}
      virtual void setHeight(const double h) {setParameter("A",h);}
      virtual void setWidth(const double w) {setParameter("frequency",1/w);}


      /// Here specify name of function as it will appear
      std::string name()const{return "Muon_ExpDecayOscTest";}

      // ** OPTIONALLY MODIFY THIS **
      /// The categories the Fit function belong to.
      /// Categories must be listed as a comma separated list.
      /// For example: "General, Muon\\Custom" which adds 
      /// this function to the category "General" and the sub-category
      /// "Muon\\Custom" 
      virtual const std::string category() const { return "C++ User Defined";}

    protected:
      virtual void functionLocal(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData);
      /// overwrite IFunction base class method, which declare function parameters
      virtual void init();

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_Muon_ExpDecayOscTest_H_*/
