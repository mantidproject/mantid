#ifndef MANTID_CURVEFITTING_FIT_H_
#define MANTID_CURVEFITTING_FIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IDomainCreator.h"

namespace Mantid
{

  namespace API
  {
    class FunctionDomain;
    class FunctionValues;
    class Workspace;
    class IFuncMinimizer;
  }

  namespace CurveFitting
  {
    /**

    A generic fitting algorithm. It fits a function to some data in a workspace.

    The static properties are:
    <UL>
      <LI>Function - The fitting function</LI>
      <LI>InputWorkspace - First input workspace with the data</LI>
      <LI>DomainType - The type of function domain to use: Simple, Sequential, or Parallel</LI>
      <LI>Ties - Optional parameter ties</LI>
      <LI>Constraints - Optional parameter constraints</LI>
      <LI>MaxIterations - Max number of iterations, default 500</LI>
      <LI>OutputStatus - A string with the output status</LI>
      <LI>OutputChi2overDoF - The final chi^2 over degrees of freedom</LI>
      <LI>Minimizer - The minimizer, default Levenberg-Marquardt</LI>
      <LI>CostFunction - The cost function , default Least squares</LI>
      <LI>CreateOutput - A flag to create output workspaces.</LI>
      <LI>Output - Optional base name for the output workspaces.</LI>
    </UL>

    After setting "Function" and "InputWorkspace" additional dynamic properties can be declared.
    Property "Function" must be set first. If it is of a multi-domain variety the algorithm will
    declare a number of properties with names "InputWorkspace_#" where # stands for a number from 1
    to n-1 where n is the number of domains required for the function. All the workspace properties
    have to be set.

    After a "InputWorkspace[_#]" property is set more dynamic proeprties can be declared. This depends
    on the functions and the type of the workspace. For example, if "Function" is IFunction1D and 
    "InputWorkspace" is a MatrixWorkspace then properties "WorkspaceIndex", "StartX", and "EndX" will
    be added.

    If the output workspaces are to be created they will have the following properties:
    <UL>
      <LI>OutputNormalisedCovarianceMatrix - A TableWorkspace with the covariance matrix</LI>
      <LI>OutputParameters - A TableWorkspace with the optimized parameters</LI>
      <LI>OutputWorkspace - Optional: some functions and input workspaces may alow to create a workspace
          with the calculated values</LI>
    </UL>

    @author Roman Tolchenov, Tessella plc
    @date 06/12/2011

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
    class DLLExport Fit : public API::Algorithm
    {
    public:
      /// Default constructor
      Fit() : API::Algorithm(),m_domainType(API::IDomainCreator::Simple) {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Fit";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Fits a function to data in a Workspace";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Optimization";}

    protected:
      
      // Overridden Algorithm methods
      void init();
      void exec();
      /// Override this method to perform a custom action right after a property was set.
      /// The argument is the property name. Default - do nothing.
      virtual void afterPropertySet(const std::string& propName);
      void setFunction();
      void addWorkspace(const std::string& workspaceNameProperty, bool addProperties = true);
      void addWorkspaces();
      /// Read domain type property and cache the value
      void setDomainType();
      void copyMinimizerOutput(const API::IFuncMinimizer& minimizer);

      /// Pointer to the fitting function
      API::IFunction_sptr m_function;
      /// Pointer to a domain creator
      boost::shared_ptr<API::IDomainCreator> m_domainCreator;
      friend class API::IDomainCreator;
      std::vector<std::string> m_workspacePropertyNames;
      /// Keep the domain type
      API::IDomainCreator::DomainType m_domainType;

    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FIT_H_*/
