#ifndef MANTID_CURVEFITTING_FIT1D_H_
#define MANTID_CURVEFITTING_FIT1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace CurveFitting
  {
    //----------------------------------------------------------------------
    // Forward Declaration
    //----------------------------------------------------------------------
    class Jacobian;
  
    /**
    Abstract base class for 1D fitting functions.

    Properties common for all fitting functions:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>

    <LI> SpectrumNumber - The spectrum to fit, using the workspace numbering of the spectra (default 0)</LI>
    <LI> StartX - Lowest value of x data array </LI>
    <LI> EndX - Highest value of x data array </LI>

    <LI> Properties defined in derived class goes here

    <LI> MaxIterations - The spectrum to fit (default 500)</LI>
    <LI> Output Status - whether the fit was successful. Direction::Output</LI>
    <LI> Output Chi^2/DoF - returns how good the fit was (default 0.0). Direction::Output</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 15/5/2009

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    class DLLExport Fit1D : public API::Algorithm
    {
    public:
      /// Default constructor
      Fit1D() : API::Algorithm() {};
      /// Destructor
      virtual ~Fit1D() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Fit1D";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

      /// Function you want to least-square fit to.
      virtual void function(const double* in, double* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData) = 0;
      /// Derivatives of function with respect to parameters you are trying to fit
      virtual void functionDeriv(const double* in, Jacobian* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData);
      /** Function you want to least-square fit to. This is the model function which is supposed to simulate 
       *  a set of "experimental" data.
       *  @param in The parameters of the model function.
       *  @param x  The argument of the function.
       */
      virtual double function(const double* in, const double& x) = 0;

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();

      /// Option for providing intelligent range starting value based e.g. on the user input parameter values
      virtual void modifyStartOfRange(double& startX) {}
      /// Option for providing intelligent range finishing value based e.g. on the user input parameter values
      virtual void modifyEndOfRange(double& endX) {}

      /// Declare additional properties other than fitting parameters
      virtual void declareAdditionalProperties(){};
      /// Called in the beginning of exec(). Custom initialization
      virtual void prepare(){};

      /// Declare parameters specific to fitting function
      virtual void declareParameters() = 0;

      /// Overload this function if the actual fitted parameters are different from
      /// those the user specifies.
      virtual void modifyInitialFittedParameters(std::vector<double>& fittedParameter);

      /// If modifyInitialFittedParameters is overloaded this method must also be overloaded
      /// to reverse the effect of modifyInitialFittedParameters before outputting the results back to the user
      virtual void modifyFinalFittedParameters(std::vector<double>& fittedParameter);

      /// Holds a copy of the value of the parameters that are actually least-squared fitted.
      std::vector<double> m_fittedParameter;

      /// Holds a copy of the names of the fitting parameters
      std::vector<std::string> m_parameterNames;

      /// Number of parameters (incuding fixed).
      size_t nParams()const{return m_parameterNames.size();}

      friend struct FitData;
    };

    
    /** Represents the Jacobian in functionDeriv. The purpose of this class is to hide from the
     *  derived Fit1D classes the fact that some of the parameters can be fixed.
     */
    class Jacobian
    {
    public:
      /**  Set a value to a Jacobian matrix element.
       *   @param iY The index of the data point.
       *   @param iP The index of the parameter. It does not depend on the number of fixed parameters in a particular fit.
       *   @param value The derivative value.
       */
      virtual void set(int iY, int iP, double value) = 0;
      /// Virtual destructor
      virtual ~Jacobian() {};
    };
    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FIT1D_H_*/
