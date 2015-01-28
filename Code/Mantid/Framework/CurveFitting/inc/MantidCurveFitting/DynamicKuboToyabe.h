#ifndef MANTID_CURVEFITTING_DYNAMICKUBOTOYABE_H_
#define MANTID_CURVEFITTING_DYNAMICKUBOTOYABE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include <cmath>

//#include "MantidAPI/ParamFunction.h"
//#include "MantidAPI/IPeakFunction.h"

//#include "MantidAPI/IFunctionMW.h"
//#include "MantidAPI/IFunction1D.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /** 
     Provide Dynamic Kubo Toyabe function interface to IPeakFunction for muon scientists.
   
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
  
     File change history is stored at: <https://github.com/mantidproject/mantid> 
     Code Documentation is available at: <http://doxygen.mantidproject.org> 
     */ 

    class DLLExport DynamicKuboToyabe :  public API::ParamFunction, public API::IFunction1D
    {
    public:

      /// Destructor
      virtual ~DynamicKuboToyabe() {}

      /// overwrite base class methods
      std::string name()const{return "DynamicKuboToyabe";}
      virtual const std::string category() const { return "Muon";}

    protected:
      virtual void function1D(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);
      virtual void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);
      virtual void init();
      virtual void setActiveParameter(size_t i, double value);

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_DYNAMICKUBOTOYABE_H_*/