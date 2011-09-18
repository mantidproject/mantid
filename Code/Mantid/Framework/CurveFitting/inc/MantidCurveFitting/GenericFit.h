#ifndef MANTID_CURVEFITTING_GENERICFIT_H_
#define MANTID_CURVEFITTING_GENERICFIT_H_
/*WIKI* 


This algorithm fits data in a [[Workspace]] with a function. The function and the initial values for its parameters are set with the Function property. The function must be compatible with the workspace.

Using the Minimizer property, Fit can be set to use different algorithms to perform the minimization. By default if the function's derivatives can be evaluated then Fit uses the GSL Levenberg-Marquardt minimizer. If the function's derivatives cannot be evaluated the GSL Simplex minimizer is used. Also, if one minimizer fails, for example the Levenberg-Marquardt minimizer, Fit may try its luck with a different minimizer. If this happens the user is notified about this and the Minimizer property is updated accordingly.

===Output===

Setting the Output property defines the names of the output workspaces. One of them is a [[TableWorkspace]] with the fitted parameter values.  If the function's derivatives can be evaluated an additional TableWorkspace is returned containing correlation coefficients in %.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/CostFuncIgnorePosPeaks.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Abstract base class for fitting functions.

    Properties common for all fitting functions:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>

    <LI> SpectrumNumber - The spectrum to fit, using the workspace numbering of the spectra (default 0)</LI>
    <LI> StartX - Lowest value of x data array </LI>
    <LI> EndX - Highest value of x data array </LI>

    <LI> Function - 

    <LI> MaxIterations - The spectrum to fit (default 500)</LI>
    <LI> OutputStatus - whether the fit was successful. Direction::Output</LI>
    <LI> OutputChi2overDoF - returns how good the fit was (default 0.0). Direction::Output</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 15/5/2009
    @author Roman Tolchenov, Tessella plc
    @date 22/12/2010

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
    class DLLExport GenericFit : public API::Algorithm
    {
    public:
      /// Default constructor
      GenericFit() : API::Algorithm(),m_function() {};
      /// Destructor
      virtual ~GenericFit();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "GenericFit";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

      /// Get the function for fitting
      boost::shared_ptr<API::IFitFunction> getFunction()const{return m_function;}

    protected:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Algorithm methods
      void init();
      void exec();

      /// calculates the derivative of a declared parameter over active parameter i
      double transformationDerivative(int i);

      /// Pointer to the fitting function
      boost::shared_ptr<API::IFitFunction> m_function;

      friend struct FitData1;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GENERICFIT_H_*/
