#ifndef MANTID_API_PARAMFUNCTION_H_
#define MANTID_API_PARAMFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/IFitFunction.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <boost/shared_array.hpp>

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

namespace Mantid
{
namespace API
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Jacobian;
class ParameterTie;
class IConstraint;
/** An interface to a function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 13/01/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTID_API ParamFunction : public virtual IFitFunction
{
public:
  /// Default constructor
  ParamFunction (){}
  /// Copy contructor
  ParamFunction (const ParamFunction &);
  /// Assignment operator
  ParamFunction & operator=(const ParamFunction &);
  /// Virtual destructor
  virtual ~ParamFunction ();

  /// Set i-th parameter
  virtual void setParameter(std::size_t, const double& value, bool explicitlySet = true);
  /// Get i-th parameter
  virtual double getParameter(std::size_t i)const;
  /// Set parameter by name.
  virtual void setParameter(const std::string& name, const double& value, bool explicitlySet = true);
  /// Get parameter by name.
  virtual double getParameter(const std::string& name)const;
  /// Total number of parameters
  virtual std::size_t nParams()const{return m_parameters.size();}
  /// Returns the index of parameter name
  virtual std::size_t parameterIndex(const std::string& name)const;
  /// Returns the index of a parameter
  //virtual std::size_t parameterIndex(const double* p)const;
  /// Returns the name of parameter i
  virtual std::string parameterName(std::size_t i)const;
  /// Checks if a parameter has been set explicitly
  virtual bool isExplicitlySet(std::size_t i)const;

  /// Number of active (in terms of fitting) parameters
  virtual std::size_t nActive()const{return m_indexMap.size();}
  /// Returns "global" index of active parameter i
  virtual std::size_t indexOfActive(std::size_t i)const;
  /// Returns the name of active parameter i
  virtual std::string nameOfActive(std::size_t i)const;

  /// Check if a declared parameter i is active
  virtual bool isActive(std::size_t i)const;
  /// Get active index for a declared parameter i
  virtual int64_t activeIndex(std::size_t i)const;
  /// Removes a declared parameter i from the list of active
  virtual void removeActive(std::size_t i);
  /// Restores a declared parameter i to the active status
  virtual void restoreActive(std::size_t i);

  /// Return parameter index from a parameter reference. Usefull for constraints and ties in composite functions
  virtual int64_t getParameterIndex(const ParameterReference& ref)const;
  /// Get the containing function
  IFitFunction* getContainingFunction(const ParameterReference& ref)const;
  /// Get the containing function
  IFitFunction* getContainingFunction(const IFitFunction* fun);

  /// Apply the ties
  virtual void applyTies();
  /// Remove all ties
  virtual void clearTies();
  virtual void removeTie(const std::string& parName){IFitFunction::removeTie(parName);}
  /// Removes i-th parameter's tie
  virtual bool removeTie(std::size_t i);
  /// Get the tie of i-th parameter
  virtual ParameterTie* getTie(std::size_t i)const;

  /// Add a constraint to function
  virtual void addConstraint(IConstraint* ic);
  /// Get constraint of i-th parameter
  virtual IConstraint* getConstraint(std::size_t i)const;
  /// Remove a constraint
  virtual void removeConstraint(const std::string& parName);
  /// Set parameters to satisfy constraints
  void setParametersToSatisfyConstraints();

  //using IFunction::removeTie;

protected:

  /// Function initialization. Declare function parameters in this method.
  virtual void init(){};
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name,double initValue = 0);

  /// Add a new tie
  virtual void addTie(ParameterTie* tie);
  /// Get the address of the parameter. For use in UserFunction with mu::Parser
  virtual double* getParameterAddress(std::size_t i);

  /// Nonvirtual member which removes all declared parameters
  void clearAllParameters();

private:
  /// The index map. m_indexMap[i] gives the total index for active parameter i
  std::vector<size_t> m_indexMap;
  /// Keeps parameter names
  std::vector<std::string> m_parameterNames;
  /// Keeps parameter values
  std::vector<double> m_parameters;
  /// Holds parameter ties as <parameter index,tie pointer> 
  std::vector<ParameterTie*> m_ties;
  /// Holds the constraints added to function
  std::vector<IConstraint*> m_constraints;
  /// Flags of explicitly set parameters
  std::vector<bool> m_explicitlySet;
  /// Temporary data storage used in functionDeriv
  //mutable boost::shared_array<double> m_tmpFunctionOutputMinusStep;
  /// Temporary data storage used in functionDeriv
  //mutable boost::shared_array<double> m_tmpFunctionOutputPlusStep;
};

} // namespace API
} // namespace Mantid


#endif /*MANTID_API_PARAMFUNCTION_H_*/
