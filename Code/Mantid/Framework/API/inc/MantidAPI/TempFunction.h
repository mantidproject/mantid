#ifndef MANTID_API_TEMPFUNCTION_H_
#define MANTID_API_TEMPFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidKernel/Exception.h"

#ifdef _WIN32
  #pragma warning( disable: 4250 )
#endif

namespace Mantid
{
namespace API
{
/** This is a temporary helper class to aid transition from IFitFunction to IFunction.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 17/11/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL TempFunction: public virtual IFunction
{
public:
  /// Constructor
  TempFunction(IFunctionMW* function);

  /// Returns the function's name
  virtual std::string name()const {return m_function->name();}
  /// Set the workspace. Make 
  /// @param ws :: Shared pointer to a workspace
  virtual void setWorkspace(boost::shared_ptr<const Workspace> ws) { UNUSED_ARG(ws) }
  /// Get the workspace
  //virtual boost::shared_ptr<const API::Workspace> getWorkspace()const {return m_function->getWorkspace();}

  /// The categories the Fit function belong to.
  /// Categories must be listed as a comma separated list.
  /// For example: "General, Muon\\Custom" which adds 
  /// a function to the category "General" and the sub-category
  /// "Muon\\Custom" 
  virtual const std::string category() const { return m_function->category();}

  virtual void function(FunctionDomain& domain)const;
  virtual void functionDeriv(FunctionDomain& domain, Jacobian& jacobian);

  /// Set i-th parameter
  virtual void setParameter(size_t i, const double& value, bool explicitlySet = true) {m_function->setParameter(i,value,explicitlySet);}
  /// Set i-th parameter description
  virtual void setParameterDescription(size_t i, const std::string& description) {m_function->setParameterDescription(i,description);}
  /// Get i-th parameter
  virtual double getParameter(size_t i)const {return m_function->getParameter(i);}
  /// Set parameter by name.
  virtual void setParameter(const std::string& name, const double& value, bool explicitlySet = true) {m_function->setParameter(name,value,explicitlySet);}
  /// Set description of parameter by name.
  virtual void setParameterDescription(const std::string& name, const std::string& description) {m_function->setParameterDescription(name,description);}
  /// Get parameter by name.
  virtual double getParameter(const std::string& name)const {return m_function->getParameter(name);}
  /// Total number of parameters
  virtual size_t nParams()const {return m_function->nParams();}
  /// Returns the index of parameter name
  virtual size_t parameterIndex(const std::string& name)const {return m_function->parameterIndex(name);}
  /// Returns the name of parameter i
  virtual std::string parameterName(size_t i)const {return m_function->parameterName(i);}
  /// Returns the description of parameter i
  virtual std::string parameterDescription(size_t i)const {return m_function->parameterDescription(i);}
  /// Checks if a parameter has been set explicitly
  virtual bool isExplicitlySet(size_t i)const {return m_function->isExplicitlySet(i);}

  /// Number of active (in terms of fitting) parameters
  virtual size_t nActive()const {return m_function->nActive();}
  /// Returns "global" index of active parameter i
  virtual size_t indexOfActive(size_t i)const {return m_function->indexOfActive(i);}
  /// Returns the name of active parameter i
  virtual std::string nameOfActive(size_t i)const {return m_function->nameOfActive(i);}
  /// Returns the name of active parameter i
  virtual std::string descriptionOfActive(size_t i)const {return m_function->descriptionOfActive(i);}

  /// Check if a declared parameter i is active
  virtual bool isActive(size_t i)const {return m_function->isActive(i);}
  /// Get active index for a declared parameter i
  virtual size_t activeIndex(size_t i)const {return m_function->activeIndex(i);}
  /// Removes a declared parameter i from the list of active
  virtual void removeActive(size_t i) {m_function->removeActive(i);}
  /// Restores a declared parameter i to the active status
  virtual void restoreActive(size_t i) {m_function->restoreActive(i);}

  /// Return parameter index from a parameter reference. Usefull for constraints and ties in composite functions
  virtual size_t getParameterIndex(const ParameterReference& ref)const {return m_function->getParameterIndex(ref);}
  /// Get a function containing the parameter refered to by the reference. In case of a simple function
  /// it will be the same as ParameterReference::getFunction(). In case of a CompositeFunction it returns
  /// a top-level function that contains the parameter. The return function itself can be a CompositeFunction
  /// @param ref :: The Parameter reference
  /// @return A pointer to the containing function
  virtual IFunction* getContainingFunction(const ParameterReference& ref)const {return m_function->getContainingFunction(ref);}
  /// The same as the method above but the argument is a function
  virtual IFunction* getContainingFunction(const IFunction* fun) {return m_function->getContainingFunction(fun);}

  /// Apply the ties
  virtual void applyTies() {m_function->applyTies();}
  /// Remove all ties
  virtual void clearTies() {m_function->clearTies();}
  /// Removes i-th parameter's tie
  virtual bool removeTie(size_t i) {return m_function->removeTie(i);}
  virtual void removeTie(const std::string& parName){IFunction::removeTie(parName);}
  /// Get the tie of i-th parameter
  virtual ParameterTie* getTie(size_t i)const {return m_function->getTie(i);}

  /// Add a constraint to function
  virtual void addConstraint(IConstraint* ic) {m_function->addConstraint(ic);}
  /// Get constraint of i-th parameter
  virtual IConstraint* getConstraint(size_t i)const {return m_function->getConstraint(i);}
  /// Remove a constraint
  virtual void removeConstraint(const std::string& parName) {m_function->removeConstraint(parName);}

protected:
  /// Function initialization. Declare function parameters in this method.
  virtual void init(){m_function->init();}
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name, double initValue = 0, const std::string& description="") 
  {
    m_function->declareParameter(name,initValue,description);
  }
  /// Add a new tie
  virtual void addTie(ParameterTie* tie) {m_function->addTie(tie);}

  /// Pointer to underlying IFitFunction
  IFunctionMW* m_function;
};

/**
 * The domain for 1D functions.
 */
class MANTID_API_DLL FunctionDomain1D: public FunctionDomain
{
public:
  FunctionDomain1D(double start, double end, size_t n);
  FunctionDomain1D(const std::vector<double>& xvalues);
  /// get an x value
  /// @param i :: Index
  double getX(size_t i) const {return m_X.at(i);}
protected:
  std::vector<double> m_X; ///< vector of function arguments
  friend class TempFunction;
};


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_TEMPFUNCTION_H_*/
