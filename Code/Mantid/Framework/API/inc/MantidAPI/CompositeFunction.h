#ifndef MANTID_API_COMPOSITEFUNCTION_H_
#define MANTID_API_COMPOSITEFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/IFitFunction.h"
#include <boost/shared_array.hpp>

namespace Mantid
{
namespace API
{
  class MatrixWorkspace;
/** A composite function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 20/10/2009

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
class DLLExport CompositeFunction : public virtual IFitFunction
{
public:
  /// Default constructor
  CompositeFunction():m_nActive(0),m_nParams(0){}
  /// Copy contructor
  CompositeFunction(const CompositeFunction&);
  ///Assignment operator
  CompositeFunction& operator=(const CompositeFunction&);
  ///Destructor
  virtual ~CompositeFunction();

              /* Overriden methods */

  /// Writes itself into a string
  std::string asString()const;

  /// Set i-th parameter
  void setParameter(size_t, const double& value, bool explicitlySet = true);
  /// Get i-th parameter
  double getParameter(size_t i)const;
  /// Set parameter by name.
  void setParameter(const std::string& name, const double& value, bool explicitlySet = true);
  /// Get parameter by name.
  double getParameter(const std::string& name)const;
  /// Total number of parameters
  std::size_t nParams()const;
  /// Returns the index of parameter name
  std::size_t parameterIndex(const std::string& name)const;
  /// Returns the index of a parameter
  //int parameterIndex(const double* p)const;
  /// Returns the name of parameter i
  std::string parameterName(std::size_t i)const;
  /// Checks if a parameter has been set explicitly
  bool isExplicitlySet(std::size_t i)const;

  /// Number of active (in terms of fitting) parameters
  std::size_t nActive()const;
  /// Value of i-th active parameter. Override this method to make fitted parameters different from the declared
  double activeParameter(int i)const;
  /// Set new value of i-th active parameter. Override this method to make fitted parameters different from the declared
  void setActiveParameter(int i, double value);
  /// Update parameters after a fitting iteration
  void updateActive(const double* in);
  /// Returns "global" index of active parameter i
  std::size_t indexOfActive(std::size_t i)const;
  /// Returns the name of active parameter i
  std::string nameOfActive(std::size_t i)const;

  /// Check if a parameter is active
  bool isActive(std::size_t i)const;
  /// Get active index for a declared parameter i
  int64_t activeIndex(std::size_t i)const;
  /// Removes a parameter from the list of active
  void removeActive(std::size_t i);
  /// Restores a declared parameter i to the active status
  void restoreActive(std::size_t i);

  /// Return parameter index from a parameter reference.
  int64_t getParameterIndex(const ParameterReference& ref)const;
  /// Get the containing function
  IFitFunction* getContainingFunction(const ParameterReference& ref)const;
  /// Get the containing function
  IFitFunction* getContainingFunction(const IFitFunction* fun);

  /// Apply the ties
  void applyTies();
  /// Remove all ties
  void clearTies();
  /// Removes i-th parameter's tie
  bool removeTie(std::size_t i);
  /// Get the tie of i-th parameter
  ParameterTie* getTie(std::size_t i)const;

  /// Overwrite IFitFunction methods
  void addConstraint(IConstraint* ic);
  /// Get constraint of i-th parameter
  virtual IConstraint* getConstraint(std::size_t i)const;
  void setParametersToSatisfyConstraints();
  /// Remove a constraint
  void removeConstraint(const std::string& parName);


             /* CompositeFunction own methods */

  /// Add a function at the back of the internal function list
  virtual std::size_t addFunction(IFitFunction* f);
  /// Returns the pointer to i-th function
  IFitFunction* getFunction(std::size_t i)const;
  /// Number of functions
  std::size_t nFunctions()const{return m_functions.size();}
  /// Remove a function
  void removeFunction(std::size_t i, bool del=true);
  /// Replace a function
  void replaceFunction(std::size_t i, IFitFunction* f);
  /// Replace a function
  void replaceOldFunction(const IFitFunction* f_old,IFitFunction* f_new);
  /// Get the function index
  std::size_t functionIndex(std::size_t i)const;
  /// Get the function index
  std::size_t functionIndexActive(std::size_t i)const;
  /// Returns the index of parameter i as it declared in its function
  std::size_t parameterLocalIndex(std::size_t i)const;
  /// Returns the name of parameter i as it declared in its function
  std::string parameterLocalName(std::size_t i)const;
  /// Check the function.
  void checkFunction();

protected:
  /// Function initialization. Declare function parameters in this method.
  void init();
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name,double initValue = 0);
  /// Add a new tie
  virtual void addTie(ParameterTie* tie);

  std::size_t paramOffset(std::size_t i)const{return m_paramOffsets[i];}
  int64_t activeOffset(std::size_t i)const{return m_activeOffsets[i];}

private:

  /// Extract function index and parameter name from a variable name
  static void parseName(const std::string& varName,std::size_t& index, std::string& name);

  /// Pointers to the included funtions
  std::vector<IFitFunction*> m_functions;
  /// Individual function parameter offsets (function index in m_functions)
  /// e.g. m_functions[i]->activeParameter(m_activeOffsets[i]+1) gives second active parameter of i-th function
  std::vector<int64_t> m_activeOffsets;
  /// Individual function parameter offsets (function index in m_functions)
  /// e.g. m_functions[i]->parameter(m_paramOffsets[i]+1) gives second declared parameter of i-th function
  std::vector<std::size_t> m_paramOffsets;
  /// Keeps the function index for each declared parameter  (parameter declared index)
  std::vector<std::size_t> m_IFitFunction;
  /// Keeps the function index for each active parameter (parameter active index)
  std::vector<std::size_t> m_IFitFunctionActive;
  /// Number of active parameters
  std::size_t m_nActive;
  /// Total number of parameters
  std::size_t m_nParams;
  /// Function counter to be used in nextConstraint
  mutable std::size_t m_iConstraintFunction;

};

/** A Jacobian for individual functions
 */
class PartialJacobian: public Jacobian
{
  Jacobian* m_J;  ///< pointer to the overall Jacobian
  std::size_t m_iP0;      ///< offset in the overall Jacobian for a particular function
  int64_t m_iaP0;      ///< offset in the active Jacobian for a particular function
public:
  /** Constructor
   * @param J :: A pointer to the overall Jacobian
   * @param iP0 :: The parameter index (declared) offset for a particular function
   * @param iap0 :: The active parameter index (declared) offset for a particular function
   */
  PartialJacobian(Jacobian* J,std::size_t iP0, int64_t iap0):m_J(J),m_iP0(iP0),m_iaP0(iap0)
  {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   * @param value :: The derivative value
   */
  void set(std::size_t iY, std::size_t iP, double value)
  {
      m_J->set(iY,m_iP0 + iP,value);
  }
  /**
   * Overridden Jacobian::get(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   */
  double get(std::size_t iY, std::size_t iP)
  {
      return m_J->get(iY,m_iP0 + iP);
  }
 /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
  *   @param value :: Value to add
  *   @param iActiveP :: The index of an active parameter.
  */
  virtual void addNumberToColumn(const double& value, const int& iActiveP) 
  {
    m_J->addNumberToColumn(value,m_iaP0+iActiveP);
  }
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEFUNCTION_H_*/
