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

namespace Mantid
{
namespace API
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;
class Jacobian;
class ParameterTie;
class IConstraint;
class ParameterReference;
/** This is an interface to a fitting function - a semi-abstarct class.
    Functions derived from IFunction can be used with the Fit algorithm.
    IFunction defines the structure of a fitting funtion.

    A function has a number of named parameters (not arguments), type double, on which it depends.
    Parameters must be declared either in the constructor or in the init() method
    of a derived class with method declareParameter(...). Method nParams() returns 
    the number of declared parameters. A parameter can be accessed either by its name
    or the index. For example in case of Gaussian the parameters can be "Height",
    "PeakCentre" and "Sigma".

    To fit a function to a set of data its parameters must be adjusted so that the difference
    between the data and the corresponding function values were minimized. This is the aim
    of the Fit algorithm. But Fit does not work with the declared parameters directly.
    Instead it uses other - active - parameters. The active parameters can be a subset of the
    declared parameters or completely different ones. The rationale for this is following.
    The fitting parameters sometimes need to be fixed during the fit or "tied" (expressed
    in terms of other parameters). In this case the active parameters will be those
    declared parameters which are not tied in any sence. Also some of the declared parameters
    can be unsuitable for the use in a fitting algorithm. In this case different active parameters
    can be used in place of the inefficient declared parameters. An example is Gaussian where
    "Sigma" makes the fit unstable. So in the fit it can be replaced with variable Weight = 1 / Sigma
    which is more efficient. The number of active parameters (returned by nActive()) cannot be
    greater than nParams(). The function which connects the active parameters with the declared ones
    must be monotonic so that the forward and backward transformations between the two sets are
    single-valued (this is my understanding). At the moment only simple one to one transformations
    of Weight - Sigma type are allowed. More complecated cases of simultaneous transformations of
    several parameters are not supported.

    The active parameters can be accessed by their index. The implementations of the access method
    for both active and declared parameters must ensure that any changes to one of them 
    immediately reflected on the other so that the two are consistent at any moment.

    IFunction declares method nameOfActive(int i) which returns the name of the declared parameter
    corresponding to the i-th active parameter. I am not completely sure in the usefulness of it.

    IFunction provides methods for tying and untying parameters. Only the declared parameters can be 
    tied. A tied parameter cannot be active. When a parameter is tied it becomes inactive.
    This implies that the number of active parameters is variable and can change at runtime.

    Method addConstraint adds constraints on possible values of a declared parameter. Constraints
    and ties are used only in fitting.

    The main method of IFunction is called function(out,xValues,nData). It calculates nData output values
    out[i] at arguments xValues[i]. Implement functionDeriv method for the function to be used with
    fitting algorithms using derivatives. functionDeriv calculates patrial derivatives of the
    function 

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
  /// Virtual destructor
  virtual ~IFunction(){}

  /// Returns the function's name
  virtual std::string name()const = 0;
  /// Writes itself into a string
  virtual std::string asString()const;
  /// The string operator
  virtual operator std::string()const{return asString();}
  /// Set the workspace
  virtual void setWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int wi,int xMin,int xMax);
  /// Iinialize the function
  virtual void initialize(){this->init();}

  /// Function you want to fit to.
  virtual void function(double* out, const double* xValues, const int& nData) = 0;
  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv(Jacobian* out, const double* xValues, const int& nData);
  /// Derivatives to be used in covariance matrix calculation. Override this method some of the fitted parameters
  /// are different form the declared ones.
  virtual void calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData);

  /// Set i-th parameter
  virtual void setParameter(int, const double& value, bool explicitlySet = true) = 0;
  /// Get i-th parameter
  virtual double getParameter(int i)const = 0;
  /// Set parameter by name.
  virtual void setParameter(const std::string& name, const double& value, bool explicitlySet = true) = 0;
  /// Get parameter by name.
  virtual double getParameter(const std::string& name)const = 0;
  /// Total number of parameters
  virtual int nParams()const = 0;
  /// Returns the index of parameter name
  virtual int parameterIndex(const std::string& name)const = 0;
  /// Returns the index of a parameter
  //virtual int parameterIndex(const double* p)const = 0;
  /// Returns the name of parameter i
  virtual std::string parameterName(int i)const = 0;
  /// Checks if a parameter has been set explicitly
  virtual bool isExplicitlySet(int i)const = 0;

  /// Number of active (in terms of fitting) parameters
  virtual int nActive()const = 0;
  /// Value of i-th active parameter. Override this method to make fitted parameters different from the declared
  virtual double activeParameter(int i)const;
  /// Set new value of i-th active parameter. Override this method to make fitted parameters different from the declared
  virtual void setActiveParameter(int i, double value);
  /// Update parameters after a fitting iteration
  virtual void updateActive(const double* in);
  /// Returns "global" index of active parameter i
  virtual int indexOfActive(int i)const = 0;
  /// Returns the name of active parameter i
  virtual std::string nameOfActive(int i)const = 0;

  /// Check if a declared parameter i is active
  virtual bool isActive(int i)const = 0;
  /// Get active index for a declared parameter i
  virtual int activeIndex(int i)const = 0;
  /// Removes a declared parameter i from the list of active
  virtual void removeActive(int i) = 0;
  /// Restores a declared parameter i to the active status
  virtual void restoreActive(int i) = 0;

  /// Return parameter index from a parameter reference. Usefull for constraints and ties in composite functions
  virtual int getParameterIndex(const ParameterReference& ref)const = 0;

  /// Tie a parameter to other parameters (or a constant)
  virtual void tie(const std::string& parName,const std::string& expr);
  /// Apply the ties
  virtual void applyTies() = 0;
  /// Removes the tie off a parameter
  virtual void removeTie(const std::string& parName);
  /// Remove all ties
  virtual void clearTies() = 0;
  /// Removes i-th parameter's tie
  virtual bool removeTie(int i) = 0;
  /// Get the tie of i-th parameter
  virtual ParameterTie* getTie(int i)const = 0;
  /// Get 

  /// Add a constraint to function
  virtual void addConstraint(IConstraint* ic) = 0;
  /// Get first constraint
  virtual IConstraint* firstConstraint()const = 0;
  /// Get next constraint
  virtual IConstraint* nextConstraint()const = 0;

  /// Set the parameters of the function to satisfy the constraints of
  /// of the function. For example
  /// for a BoundaryConstraint this if param value less than lower boundary
  /// it is set to that value and vice versa for if the param value is larger
  /// than the upper boundary value.
  virtual void setParametersToSatisfyConstraints() {};

  /// Returns the number of attributes associated with the function
  virtual int nAttributes()const{return 0;}
  /// Returns a list of attribute names
  virtual std::vector<std::string> getAttributeNames()const{return std::vector<std::string>();}
  /// Return a value of attribute attName
  virtual std::string getAttribute(const std::string& attName)const{return "";}
  /// Set a value to attribute attName
  virtual void setAttribute(const std::string& attName,const std::string& value){}
  /// Check if attribute attName exists
  virtual bool hasAttribute(const std::string& attName)const{return false;}

protected:

  /// Function initialization. Declare function parameters in this method.
  virtual void init(){};
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name,double initValue = 0) = 0;

  /// Create an instance of a tie without actually tying it to anything
  virtual ParameterTie* createTie(const std::string& parName);
  /// Add a new tie
  virtual void addTie(ParameterTie* tie) = 0;

  friend class ParameterTie;
  friend class CompositeFunction;

  /// Shared pointer to the workspace
  boost::shared_ptr<const API::MatrixWorkspace> m_workspace;
  /// Spectrum index
  int m_workspaceIndex;
  /// Lower bin index
  int m_xMinIndex;
  /// Upper bin index
  int m_xMaxIndex;

};


/** Represents the Jacobian in functionDeriv. 
*  It is abstract to abstract from any GSL
*/
class Jacobian
{
public:
  /**  Set a value to a Jacobian matrix element.
  *   @param iY The index of a data point.
  *   @param iP The index of a declared parameter.
  *   @param value The derivative value.
  */
  virtual void set(int iY, int iP, double value) = 0;

  /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
  *   @param value Value to add
  *   @param iActiveP The index of an active parameter.
  */
  virtual void addNumberToColumn(const double& value, const int& iActiveP) 
  {
    throw Kernel::Exception::NotImplementedError("No addNumberToColumn() method of Jacobian provided");
  }

  /// Virtual destructor
  virtual ~Jacobian() {};
protected:
};

/// Overload operator <<
DLLExport std::ostream& operator<<(std::ostream& ostr,const IFunction& f);

} // namespace API
} // namespace Mantid

/**
 * Macro for declaring a new type of function to be used with the FunctionFactory
 */
#define DECLARE_FUNCTION(classname) \
        namespace { \
	Mantid::Kernel::RegistrationHelper register_function_##classname( \
  ((Mantid::API::FunctionFactory::Instance().subscribe<classname>(#classname)) \
	, 0)); \
	}

#endif /*MANTID_API_IFUNCTION_H_*/
