#ifndef MANTID_API_IFUNCTION_H_
#define MANTID_API_IFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/FunctionFactory.h"
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

namespace DataObjects
{
  class Workspace2D;
}

namespace API
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Jacobian;
class ParameterTie;
/** An interface to a function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

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
class DLLExport IFunction
{
public:
  /// Default constructor
  IFunction(){}
  /// Copy contructor
  IFunction(const IFunction&);
  /// Assignment operator
  IFunction& operator=(const IFunction&);
  /// Virtual destructor
  virtual ~IFunction();

  /// Initialize the function providing it the workspace
  virtual void initialize(boost::shared_ptr<const DataObjects::Workspace2D> workspace,int wi,int xMin,int xMax);

  /// Function you want to fit to.
  virtual void function(double* out, const double* xValues, const int& nData) = 0;
  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv(Jacobian* out, const double* xValues, const int& nData);
  /// Derivatives to be used in covariance matrix calculation. Override this method some of the fitted parameters
  /// are different form the declared ones.
  virtual void calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData);

  /// Address of i-th parameter
  virtual double& parameter(int);
  /// Address of i-th parameter
  virtual double parameter(int i)const;
  /// Get parameter by name.
  virtual double& getParameter(const std::string& name);
  /// Get parameter by name.
  virtual double getParameter(const std::string& name)const;
  /// Total number of parameters
  virtual int nParams()const{return m_parameters.size();};
  /// Returns the index of parameter name
  virtual int parameterIndex(const std::string& name)const;
  /// Returns the name of parameter i
  virtual std::string parameterName(int i)const;

  /// Number of active (in terms of fitting) parameters
  virtual int nActive()const{return m_indexMap.size();}
  /// Value of i-th active parameter. Override this method to make fitted parameters different from the declared
  virtual double activeParameter(int i)const;
  /// Set new value of i-th active parameter. Override this method to make fitted parameters different from the declared
  virtual void setActiveParameter(int i, double value);
  /// Update parameters after a fitting iteration
  virtual void updateActive(const double* in);
  /// Returns "global" index of active parameter i
  virtual int indexOfActive(int i)const;
  /// Returns the name of active parameter i
  virtual std::string nameOfActive(int i)const;

  /// Check if a declared parameter i is active
  virtual bool isActive(int i)const;
  /// Removes a declared parameter i from the list of active
  virtual void removeActive(int i);
  /// Get active index for a declared parameter i
  virtual int activeIndex(int i)const;

  /// Tie a parameter to other parameters (or a constant)
  virtual void tie(const std::string& parName,const std::string& expr);
  /// Apply the ties
  virtual void applyTies();

protected:
  /// Function initialization. Declare function parameters in this method.
  virtual void init(){};
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name,double initValue = 0);

  /// Shared pointer to the workspace
  boost::shared_ptr<const DataObjects::Workspace2D> m_workspace;
  /// Spectrum index
  int m_workspaceIndex;
  /// Lower bin index
  int m_xMinIndex;
  /// Upper bin index
  int m_xMaxIndex;

private:
  /// The index map. m_indexMap[i] gives the total index for active parameter i
  std::vector<int> m_indexMap;
  /// Keeps parameter names
  std::vector<std::string> m_parameterNames;
  /// Keeps parameter values
  std::vector<double> m_parameters;
  /// Holds parameter ties
  std::vector<std::pair<int,ParameterTie*> > m_ties;
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

/**
 * Macro for declaring a new type of function to be used with the FunctionFactory
 */
#define DECLARE_FUNCTION(classname) \
        namespace { \
	Mantid::Kernel::RegistrationHelper register_alg_##classname( \
  ((Mantid::API::FunctionFactory::Instance().subscribe<classname>(#classname)) \
	, 0)); \
	}

#endif /*MANTID_API_IFUNCTION_H_*/
