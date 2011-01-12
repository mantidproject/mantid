#ifndef MANTID_API_COMPOSITEFUNCTION_H_
#define MANTID_API_COMPOSITEFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"

namespace Mantid
{
namespace API
{
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
class DLLExport CompositeFunction : public IFunction
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

  void setWorkspace(boost::shared_ptr<Workspace> ws,const std::string& slicing);
  /// Set the workspace
  void setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int spec,int xMin,int xMax);
  /// Returns the function's name
  std::string name()const{return "CompositeFunction";}
  /// Writes itself into a string
  std::string asString()const;

  /// Function you want to fit to.
  void function(double* out, const double* xValues, const int& nData)const;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData);
  /// Derivatives to be used in covariance matrix calculation. 
  void calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData);

  /// Set i-th parameter
  void setParameter(int, const double& value, bool explicitlySet = true);
  /// Get i-th parameter
  double getParameter(int i)const;
  /// Set parameter by name.
  void setParameter(const std::string& name, const double& value, bool explicitlySet = true);
  /// Get parameter by name.
  double getParameter(const std::string& name)const;
  /// Total number of parameters
  int nParams()const;
  /// Returns the index of parameter name
  int parameterIndex(const std::string& name)const;
  /// Returns the index of a parameter
  //int parameterIndex(const double* p)const;
  /// Returns the name of parameter i
  std::string parameterName(int i)const;
  /// Checks if a parameter has been set explicitly
  bool isExplicitlySet(int i)const;

  /// Number of active (in terms of fitting) parameters
  int nActive()const;
  /// Value of i-th active parameter. Override this method to make fitted parameters different from the declared
  double activeParameter(int i)const;
  /// Set new value of i-th active parameter. Override this method to make fitted parameters different from the declared
  void setActiveParameter(int i, double value);
  /// Update parameters after a fitting iteration
  void updateActive(const double* in);
  /// Returns "global" index of active parameter i
  int indexOfActive(int i)const;
  /// Returns the name of active parameter i
  std::string nameOfActive(int i)const;

  /// Check if a parameter is active
  bool isActive(int i)const;
  /// Get active index for a declared parameter i
  int activeIndex(int i)const;
  /// Removes a parameter from the list of active
  void removeActive(int i);
  /// Restores a declared parameter i to the active status
  void restoreActive(int i);

  /// Return parameter index from a parameter reference.
  int getParameterIndex(const ParameterReference& ref)const;
  /// Get the containing function
  IFitFunction* getContainingFunction(const ParameterReference& ref)const;
  /// Get the containing function
  IFitFunction* getContainingFunction(const IFitFunction* fun);

  /// Apply the ties
  void applyTies();
  /// Remove all ties
  void clearTies();
  /// Removes i-th parameter's tie
  bool removeTie(int i);
  /// Get the tie of i-th parameter
  ParameterTie* getTie(int i)const;

  /// Overwrite IFunction methods
  void addConstraint(IConstraint* ic);
  /// Get constraint of i-th parameter
  virtual IConstraint* getConstraint(int i)const;
  void setParametersToSatisfyConstraints();
  /// Remove a constraint
  void removeConstraint(const std::string& parName);

             /* CompositeFunction own methods */

  /// Add a function at the back of the internal function list
  virtual int addFunction(IFunction* f);
  /// Returns the pointer to i-th function
  IFunction* getFunction(int i)const;
  /// Number of functions
  int nFunctions()const{return static_cast<int>(m_functions.size());}
  /// Remove a function
  void removeFunction(int i, bool del=true);
  /// Replace a function
  void replaceFunction(int i,IFunction* f);
  /// Replace a function
  void replaceFunction(const IFunction* f_old,IFunction* f_new);
  /// Get the function index
  int functionIndex(int i)const;
  /// Get the function index
  int functionIndexActive(int i)const;
  /// Returns the index of parameter i as it declared in its function
  int parameterLocalIndex(int i)const;
  /// Returns the name of parameter i as it declared in its function
  std::string parameterLocalName(int i)const;
  /// Check the function.
  void checkFunction();

  void setUpNewStuff(boost::shared_array<double> xs = boost::shared_array<double>(),boost::shared_array<double> weights = boost::shared_array<double>());

protected:
  /// Function initialization. Declare function parameters in this method.
  void init();
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name,double initValue = 0);
  /// Add a new tie
  virtual void addTie(ParameterTie* tie);

private:

  /// Extract function index and parameter name from a variable name
  static void parseName(const std::string& varName,int& index, std::string& name);

  /// Pointers to the included funtions
  std::vector<IFunction*> m_functions;
  /// Individual function parameter offsets (function index in m_functions)
  /// e.g. m_functions[i]->activeParameter(m_activeOffsets[i]+1) gives second active parameter of i-th function
  std::vector<int> m_activeOffsets;
  /// Individual function parameter offsets (function index in m_functions)
  /// e.g. m_functions[i]->parameter(m_paramOffsets[i]+1) gives second declared parameter of i-th function
  std::vector<int> m_paramOffsets;
  /// Keeps the function index for each declared parameter  (parameter declared index)
  std::vector<int> m_iFunction;
  /// Keeps the function index for each active parameter (parameter active index)
  std::vector<int> m_iFunctionActive;
  /// Number of active parameters
  int m_nActive;
  /// Total number of parameters
  int m_nParams;
  /// Function counter to be used in nextConstraint
  mutable int m_iConstraintFunction;

  //friend class ParameterTie;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEFUNCTION_H_*/
