#ifndef MANTID_CURVEFITTING_ABRAGAM_H_
#define MANTID_CURVEFITTING_ABRAGAM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
//#include "MantidAPI/IPeakFunction.h"

#include "MantidAPI/IFunctionMW.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /** 
    Provide Abragam fitting function for muon scientists
   
     @author Karl Palmen, ISIS, RAL 
     @date 21/03/2012 
  
     Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory 
  
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

    class DLLExport Abragam : public API::ParamFunction, public API::IFunctionMW
    {
    public:

      /// Destructor
      virtual ~Abragam() {}

      /// overwrite IFunction base class methods
      std::string name()const{return "Abragam";}

      /// overwrite IFunction base class methods
      virtual const std::string category() const { return "Muon";}
    protected:
      virtual void functionMW(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDerivMW(API::Jacobian* out, const double* xValues, const size_t nData);
      virtual void setActiveParameter(size_t i,double value);

      /// overwrite IFunction base class method that declares function parameters 
      virtual void init(); 

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ABRAGAM_H_*/
