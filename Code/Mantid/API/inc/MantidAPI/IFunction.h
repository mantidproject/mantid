#ifndef MANTID_API_IFUNCTION_H_
#define MANTID_API_IFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FunctionFactory.h"
#include "boost/shared_ptr.hpp"
#include "boost/variant.hpp"
#include <string>
#include <vector>

namespace Mantid
{
namespace API
{
#ifndef IGNORE_IFUNCTION_ARGUMENT
#define IGNORE_IFUNCTION_ARGUMENT(x)
#endif
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;
class Jacobian;
class ParameterTie;
class IConstraint;
class ParameterReference;
class FunctionHandler;
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
    function with respect to the fitting parameters.

    Any non-fitting parameters can be implemented as attributes (class IFunction::Attribute). 
    An attribute can have one of three types: std::string, int, or double. The type is set at construction
    and cannot be changed later. To read or write the attributes there are two ways. If the type
    is known the type specific accessors can be used, e.g. asString(), asInt(). Otherwise the
    IFunction::AttributeVisitor can be used. It provides alternative virtual methods to access 
    attributes of each type. When creating a function from a string (using FunctionFactory::creaeInitialized(...))
    the attributes must be set first, before any fitting parameter, as the number and names of the parameters
    can depend on the attributes.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

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
class DLLExport IFunction
{
public:

  /**
   * Atribute visitor class. It provides a separate access method
   * for each attribute type. When applied to a particular attribue
   * the appropriate method will be used. The child classes must
   * implement the virtual AttributeVisitor::apply methods. See 
   * implementation of Attribute::value() method for an example.
   */
  template<typename T = void>
  class DLLExport AttributeVisitor: public boost::static_visitor<T>
  {
  public:
    /// Virtual destructor
    virtual ~AttributeVisitor() {}
    /// implements static_visitor's operator() for std::string
    T operator()(std::string& str)const{return apply(str);}
    /// implements static_visitor's operator() for double
    T operator()(double& d)const{return apply(d);}
    /// implements static_visitor's operator() for int
    T operator()(int& i)const{return apply(i);}
  protected:
    /// Implement this mathod to access attribute as string
    virtual T apply(std::string&)const = 0;
    /// Implement this mathod to access attribute as double
    virtual T apply(double&)const = 0;
    /// Implement this mathod to access attribute as int
    virtual T apply(int&)const = 0;
  };

  /**
   * Const version of AttributeVisitor. 
   */
  template<typename T = void>
  class DLLExport ConstAttributeVisitor: public boost::static_visitor<T>
  {
  public:
    /// Virtual destructor
    virtual ~ConstAttributeVisitor() {}
    /// implements static_visitor's operator() for std::string
    T operator()(std::string& str)const{return apply(str);}
    /// implements static_visitor's operator() for double
    T operator()(double& d)const{return apply(d);}
    /// implements static_visitor's operator() for int
    T operator()(int& i)const{return apply(i);}
  protected:
    /// Implement this mathod to access attribute as string
    virtual T apply(const std::string& str)const = 0;
    /// Implement this mathod to access attribute as double
    virtual T apply(const double& d)const = 0;
    /// Implement this mathod to access attribute as int
    virtual T apply(const int& i)const = 0;
  };

  /// Attribute is a non-fitting parameter.
  /// It can be one of the types: std::string, int, or double
  /// Examples: file name, polinomial order
  class DLLExport Attribute
  {
  public:
    /// Create string attribute
    explicit Attribute(const std::string& str, bool quoteValue=false):m_data(str), m_quoteValue(quoteValue) {}
    /// Create int attribute
    explicit Attribute(const int& i):m_data(i){}
    /// Create double attribute
    explicit Attribute(const double& d):m_data(d){}
    /// Apply an attribute visitor
    template<typename T>
    T apply(AttributeVisitor<T>& v){return boost::apply_visitor(v,m_data);}
    /// Apply a const attribute visitor
    template<typename T>
    T apply(ConstAttributeVisitor<T>& v)const{return boost::apply_visitor(v,m_data);}
    /// Returns type of the attribute
    std::string type()const;
    /// Returns the attribute value as a string
    std::string value()const;
    /// Returns string value if attribute is a string, throws exception otherwise
    std::string asString()const;
    /// Returns a string value that is guarenteed to be quoted for use in places where the string is used as the displayed value.
    std::string asQuotedString() const;
    /// Returns a string value that is guarenteed to be unquoted.
    std::string asUnquotedString() const;
    /// Returns int value if attribute is a int, throws exception otherwise
    int asInt()const;
    /// Returns double value if attribute is a double, throws exception otherwise
    double asDouble()const;
    /// Sets new value if attribute is a string
    void setString(const std::string& str);
    /// Sets new value if attribute is a double
    void setDouble(const double&);
    /// Sets new value if attribute is a int
    void setInt(const int&);
    /// Set value from a string.
    void fromString(const std::string& str);
  private:
    /// The data holder as boost variant
    mutable boost::variant<std::string,int,double> m_data;
    /// Flag indicating if the string value should be returned quoted
    bool m_quoteValue;
  };

  //---------------------------------------------------------//

  /// Constructor
  IFunction():m_handler(NULL){}
  /// Virtual destructor
  virtual ~IFunction();

  /// Returns the function's name
  virtual std::string name()const = 0;
  /// Writes itself into a string
  virtual std::string asString()const;
  /// The string operator
  virtual operator std::string()const{return asString();}
  /// Set the workspace
  virtual void setWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int wi,int xMin,int xMax);
  /// Get the workspace
  virtual boost::shared_ptr<const API::MatrixWorkspace> getWorkspace()const{return m_workspace;}
  /// Get workspace index
  virtual int getWorkspaceIndex()const{return m_workspaceIndex;}
  /// Iinialize the function
  virtual void initialize(){this->init();}

  /// Function you want to fit to.
  virtual void function(double* out, const double* xValues, const int& nData)const = 0;
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
  /// Get a function containing the parameter refered to by the reference. In case of a simple function
  /// it will be the same as ParameterReference::getFunction(). In case of a CompositeFunction it returns
  /// a top-level function that contains the parameter. The return function itself can be a CompositeFunction
  /// @param ref The Parameter reference
  /// @return A pointer to the containing function
  virtual IFunction* getContainingFunction(const ParameterReference& ref)const = 0;
  /// The same as the method above but the argument is a function
  virtual IFunction* getContainingFunction(const IFunction* fun) = 0;

  /// Tie a parameter to other parameters (or a constant)
  virtual ParameterTie* tie(const std::string& parName,const std::string& expr);
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

  /// Add a constraint to function
  virtual void addConstraint(IConstraint* ic) = 0;
  /// Get constraint of i-th parameter
  virtual IConstraint* getConstraint(int i)const = 0;
  /// Remove a constraint
  virtual void removeConstraint(const std::string& parName) = 0;

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
  virtual Attribute getAttribute(const std::string& attName)const
  {
    throw std::invalid_argument("Attribute "+attName+" not found in function "+this->name());
  }
  /// Set a value to attribute attName
  virtual void setAttribute(const std::string& attName,const Attribute& )
  {
    throw std::invalid_argument("Attribute "+attName+" not found in function "+this->name());
  }
  /// Check if attribute attName exists
  virtual bool hasAttribute(const std::string& IGNORE_IFUNCTION_ARGUMENT(attName))const{return false;}

  /// Set a function handler
  void setHandler(FunctionHandler* handler);
  /// Return the handler
  FunctionHandler* getHandler()const{return m_handler;}

protected:

  /// Function initialization. Declare function parameters in this method.
  virtual void init(){};
  /// Declare a new parameter
  virtual void declareParameter(const std::string& name,double initValue = 0) = 0;

  /// Create an instance of a tie without actually tying it to anything
  virtual ParameterTie* createTie(const std::string& parName);
  /// Add a new tie
  virtual void addTie(ParameterTie* tie) = 0;

  /// Convert a value from one unit (inUnit) to unit defined in workspace (ws) 
  double convertValue(double value, Kernel::Unit_sptr& inUnit, 
                      boost::shared_ptr<const MatrixWorkspace> ws,
                      int wsIndex);

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

  /// Pointer to a function handler
  FunctionHandler* m_handler;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
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

  ///@cond do not document
  /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
  *   @param value Value to add
  */
  virtual void addNumberToColumn(const double& IGNORE_IFUNCTION_ARGUMENT(value), const int& IGNORE_IFUNCTION_ARGUMENT(iActiveP)) 
  {
    throw Kernel::Exception::NotImplementedError("No addNumberToColumn() method of Jacobian provided");
  }
  ///@endcond

  /// Virtual destructor
  virtual ~Jacobian() {};
protected:
};

/// Overload operator <<
DLLExport std::ostream& operator<<(std::ostream& ostr,const IFunction& f);

/**
 * Classes inherited from FunctionHandler will handle the function.
 * The intended purpose is to help with displaying nested composite
 * functions in a tree view. This way a display handler shows only
 * single function and there is no need to duplicate the function tree
 * structure.
 */
class FunctionHandler
{
public:
  /// Constructor
  FunctionHandler(IFunction* fun):m_fun(fun){}
  /// Virtual destructor
  virtual ~FunctionHandler(){}
  /// abstract init method. It is called after setting handler to the function
  virtual void init() = 0;
  /// Return the handled function
  const IFunction* function()const{return m_fun;}
protected:
  IFunction* m_fun;///< pointer to the handled function
};

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
