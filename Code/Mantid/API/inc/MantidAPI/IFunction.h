#ifndef MANTID_API_IFUNCTION_H_
#define MANTID_API_IFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>

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
class MatrixWorkspace;
class Jacobian;

/** An interface to a function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport IFunction
{
public:
  /// Function initialization. Declare function parameters in this method.
  virtual void init() = 0;
  /// Function you want to fit to.
  virtual void function(double* out, const double* xValues, const int& nData) = 0;
  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv(Jacobian* out, const double* xValues, const int& nData) = 0;
  /// Derivatives to be used in covariance matrix calculation
  virtual void calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData);

  /// Address of i-th parameter
  double& parameter(int);
  /// Address of i-th parameter
  double parameter(int i)const;
  /// Get parameter by name.
  double& getParameter(const std::string& name);
  /// Get parameter by name.
  double getParameter(const std::string& name)const;
  /// Total number of parameters
  int nParams()const{return m_parameters.size();};
  /// Returns the name of parameter i
  std::string parameterName(int i)const;

  /// Number of active (in terms of fitting) parameters
  int nActive()const{return m_indexMap.size() ? m_indexMap.size() : nParams();}
  /// Value of i-th active parameter
  virtual double activeParameter(int i);
  /// Set new value of i-th active parameter
  virtual void setActiveParameter(int i, double value);
  /// Update parameters after a fitting iteration
  void updateActive(const double* in);
  /// Returns "global" index of active parameter i
  int indexOfActive(int i)const;
  /// Returns the name of active parameter i
  std::string nameOfActive(int i)const;

protected:
  /// Declare a new parameter
  void declareParameter(const std::string& name,double initValue = 0);

private:
  /// The index map. m_indexMap[i] gives the total index for active parameter i
  std::vector<int> m_indexMap;
  std::vector<std::string> m_parameterNames;
  std::vector<double> m_parameters;
};

/** Represents the Jacobian in functionDeriv. 
*  It is abstract to abstract from any GSL
*/
class Jacobian
{
public:
  /**  Set a value to a Jacobian matrix element.
  *   @param iY The index of a data point.
  *   @param iP The index of an active? parameter.
  *   @param value The derivative value.
  */
  virtual void set(int iY, int iP, double value) = 0;
  /// Virtual destructor
  virtual ~Jacobian() {};
protected:
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IFUNCTION_H_*/
