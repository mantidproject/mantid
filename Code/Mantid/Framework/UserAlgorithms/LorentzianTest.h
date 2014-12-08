#ifndef MANTID_CURVEFITTING_LORENTZIANTEST_H_
#define MANTID_CURVEFITTING_LORENTZIANTEST_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    This is a copy of the Lorentzian peak shape function in Mantid. 

    This copy is here used to demonstrate how to add a user defined Fit Function.

    The instructions here complement the instruction at <http://http://www.mantidproject.org/Writing_a_Fit_Function>

    1) Rename LorentzianTest.h and LorentzianTest.cpp to the name of your new Fit Function, say MyFit.h and MyFit.cpp
    2) Open MyFit.h and substitute all instances of LORENTZIANTEST with MYFIT and LorentzianTest with MyFit. 
    3) Open MyFit.cpp and substitute LorentzianTest with MyFit
    4) Check that MyFit works, see <http://http://www.mantidproject.org/Writing_a_Fit_Function> for instructions
    5) Now the fun bit, modify the sections in MyFit.h and MyFit.cpp entitled ** MODIFY THIS **


    Note for information about the Mantid implementation of the Lorentzian Fit Function see 
    <http://http://www.mantidproject.org/Lorentzian>. It has three fitting 
    parameters and its formula is: Height*( HWHM^2/((x-PeakCentre)^2+HWHM^2) ).

    In more detail the Lorentzian fitting parameters are:
    <UL>
    <LI> Height - height of peak (default 0.0)</LI>
    <LI> PeakCentre - centre of peak (default 0.0)</LI>
    <LI> HWHM - half-width half-maximum (default 0.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 21/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport LorentzianTest : public API::IPeakFunction
    {
    public:

      // ** MODIFY THIS **
      // These methods are used by the MantidPlot GUI to allow users to
      // graphically adjust the e.g. the peak width and translate such an
      // adjustments to width fit parameter(s)
      // 
      // This implementation of a Lorentzian has three fitting parameters
      // PeakCentre, Height and HWHM as described above and declared in 
      // the init() method in the .cpp file
      // 
      // From the fitting parameters of your function use these to give
      // a best estimate of how to calculate the centre (in the case of
      // an exponential function this might be set to the start value of 
      // the function), the height and width. Note these should just be
      // best estimates and is only used Fit GUI for given a better user
      // feel when using your fit function
      //
      // Initially you may set these as follows:
      //
      //       virtual double centre()const {return 0.0;}
      //       virtual double height()const {return 1.0;}
      //       virtual double width()const {return 0.0;}
      //       virtual void setCentre(const double c) {}
      //       virtual void setHeight(const double h) {}
      //       virtual void setWidth(const double w) {}
      //
      // I.e. replace the below 6 lines of code with the above 6 lines
      virtual double centre()const {return getParameter("PeakCentre");}
      virtual double height()const {return getParameter("Height");}
      virtual double fwhm()const {return 2*getParameter("HWHM");}
      virtual void setCentre(const double c) {setParameter("PeakCentre",c);}
      virtual void setHeight(const double h) {setParameter("Height",h);}
      virtual void setFwhm(const double w) {setParameter("HWHM",w/2.0);}


      // ** MODIFY THIS **
      /// The name of the fitting function
      std::string name()const{return "LorentzianTest";}

      // ** OPTIONALLY MODIFY THIS **
      /// The categories the Fit function belong to.
      /// Categories must be listed as a comma separated list.
      /// For example: "General, Muon\\Custom" which adds 
      /// this function to the category "General" and the sub-category
      /// "Muon\\Custom" 
      virtual const std::string category() const { return "C++ User Defined";}

      virtual const std::string summary() const { return "C++ User defined algorithm."; }

    protected:
      virtual void functionLocal(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData);
      virtual void init();

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LORENTZIAN_H_*/
