#ifndef MANTID_CURVEFITTING_FIT_H_
#define MANTID_CURVEFITTING_FIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
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
    <LI> Output Status - whether the fit was successful. Direction::Output</LI>
    <LI> Output Chi^2/DoF - returns how good the fit was (default 0.0). Direction::Output</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 15/5/2009

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
    class DLLExport Fit : public API::Algorithm
    {
    public:
      /// Default constructor
      Fit() : API::Algorithm(),m_function(NULL) {};
      /// Destructor
      virtual ~Fit();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Fit";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

      /// Function you want to fit to.
      //virtual void function(const double* in, double* out, const double* xValues, const int& nData);
      /// Derivatives of function with respect to parameters you are trying to fit
      //virtual void functionDeriv(const double* in, API::Jacobian* out, const double* xValues, const int& nData);

      /// Get the function for fitting
      API::IFunction* getFunction()const{return m_function;}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();
      void exec1();

      /// Set a function for fitting
      void setFunction(API::IFunction* fun);

      /// Option for providing intelligent range starting value based e.g. on the user input parameter values
      virtual void modifyStartOfRange(double& startX)
      {
        (void) startX;
      }

      /// Option for providing intelligent range finishing value based e.g. on the user input parameter values
      virtual void modifyEndOfRange(double& endX)
      {
        (void) endX;
      }

      /** Called after the data ranged has been determined but before the fitting starts.
       *  For example may be used to create wavelength array for each TOF data-point.
       *  Number of data point to fit over are m_maxX-m_minX.
       * 
       *  @param m_minX Start array index.
       *  @param m_maxX End array index.
       */
      virtual void afterDataRangedDetermined(const int& m_minX, const int& m_maxX)
      {
        (void) m_minX;
        (void) m_maxX;
      };

      // Process input parameters and create the fitting function.
      void processParameters();

      /// calculates the derivative of a declared parameter over active parameter i
      double transformationDerivative(int i);

      /// Number of parameters.
      size_t nActive()const{return m_function->nActive();}

      /// Pointer to the fitting function
      API::IFunction* m_function;

      /// Function initialization string
      std::string m_function_input;

      friend struct FitData1;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FIT_H_*/
