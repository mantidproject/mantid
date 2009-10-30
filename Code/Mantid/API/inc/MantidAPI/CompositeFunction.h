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

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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

  /// Function initialization. Declare function parameters in this method.
  void init();
  /// Function you want to fit to.
  void function(double* out, const double* xValues, const int& nData);
  /// Derivatives of function with respect to active parameters
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData);
  /// Derivatives to be used in covariance matrix calculation. 
  void calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData);

  /// Address of i-th parameter
  double& parameter(int);
  /// Address of i-th parameter
  double parameter(int i)const;
  /// Get parameter by name.
  double& getParameter(const std::string& name);
  /// Get parameter by name.
  double getParameter(const std::string& name)const;
  /// Total number of parameters
  int nParams()const;
  /// Returns the index of parameter name
  int parameterIndex(const std::string& name)const;
  /// Returns the name of parameter i
  std::string parameterName(int i)const;

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
  /// Removes a parameter from the list of active
  void removeActive(int i);
  /// Get active index for a declared parameter i
  int activeIndex(int i)const;

  /// Add a function
  void addFunction(IFunction* f);
  /// Returns the pointer to i-th function
  IFunction* getFunction(int i)const;
  /// Number of functions
  int nFunctions()const{return m_functions.size();}

private:
  double m_tst;
  /// Get the function index
  int functionIndex(int i)const;
  /// Get the function index
  int functionIndexActive(int i)const;
  /// Extract function index and parameter name from a variable name
  static void parseName(const std::string& varName,int& index, std::string& name);

  /// Pointers to the included funtions
  std::vector<IFunction*> m_functions;
  /// Individual function parameter offsets
  std::vector<int> m_activeOffsets;
  /// Individual function parameter offsets
  std::vector<int> m_paramOffsets;
  /// Keeps the function number for each declared parameter
  std::vector<int> m_iFunction;
  /// Keeps the function number for each active parameter
  std::vector<int> m_iFunctionActive;
  /// Number of active parameters
  int m_nActive;
  /// Total number of parameters
  int m_nParams;

  //friend class ParameterTie;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEFUNCTION_H_*/
