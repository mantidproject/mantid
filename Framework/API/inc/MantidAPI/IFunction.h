// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/WarningSuppressions.h"

#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#include <memory>
#endif

#include <string>
#include <utility>

#include <vector>

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ProgressBase;
} // namespace Kernel
namespace API {
class Workspace;
class MatrixWorkspace;
class FunctionHandler;

/**
 * Attribute visitor structure supporting lambda expressions
 * Example usage: AttributeLambdaVisitor{[](const int val) {...}, [] (const
 * double val) {}} would create a visitor capable of "visiting" an integer and
 * double attribute*
 * It functions by inheriting the () operator defined in each lambda
 */
template <class... Ts> struct AttributeLambdaVisitor : Ts... { using Ts::operator()...; };
template <class... Ts> AttributeLambdaVisitor(Ts...) -> AttributeLambdaVisitor<Ts...>;

/** This is an interface to a fitting function - a semi-abstarct class.
    Functions derived from IFunction can be used with the Fit algorithm.
    IFunction defines the structure of a fitting funtion.

    A function has a number of named parameters (not arguments), type double, on
   which it depends.
    Parameters must be declared either in the constructor or in the init()
   method
    of a derived class with method declareParameter(...). Method nParams()
   returns
    the number of parameters. A parameter can be accessed either by its name
    or the index. For example in case of Gaussian the parameters can be
   "Height",
    "PeakCentre" and "Sigma".

    The main method of IFunction is called function(const
   FunctionDomain&,FunctionValues&).
    It takes a set of function arguments via interface FunctionDomain,
   calculates the values, and
    returns them via the FunctionValues. The derived classes must implement this
   method.

    Implement functionDeriv method for the function to be used with
    fitting algorithms using derivatives. functionDeriv calculates patrial
   derivatives of the
    function with respect to the fitting parameters. The default implementation
   uses numeric differentiation.

    To fit a function to a set of data its parameters must be adjusted so that
   the difference
    between the data and the corresponding function values were minimized. This
   is the aim
    of the Fit algorithm. But Fit does not work with the declared parameters
   directly.
    Instead it uses other - active - parameters. In simple case the active
   parameters are the
    same as the declared ones. But they can be overidden if the declared
   parameters make fit unstable.
    There are as many active parameters as there are the declared ones. A
   one-to-one transformation
    must exist between the active and the declared parameters. Overide
   activeParameter and
    setActiveParameter methods to implement this transformation. An example is
   Gaussian where
    "Sigma" makes the fit unstable. So in the fit it is replaced with variable
   Weight = 1 / Sigma
    which is more efficient.

    The active parameters can be accessed by their index. The implementations of
   the access method
    for both active and declared parameters must ensure that any changes to one
   of them
    immediately reflected on the other so that the two sets are consistent at
   any moment.

    IFunction declares method nameOfActive(int i) which returns the name of the
   declared parameter
    corresponding to the i-th active parameter. I am not completely sure in the
   usefulness of it.

    The declared parameters can be made fixed in a fit with method fix(). If a
   parameter is fixed a fit
    shouldn't change its value unless it is also tied to values of other
   parameters. Implementations of
    active parameters must ensure this behaviour.

    When a declared parameter is made fixed one of the active parameters must
   become inactive. isActive(i)
    method must return false for it. In case of declared == active the fixed
   parameter becomes inactive.
    Classes overriding active parameters must ensure that number of inactive
   parameters == number of
    fixed declared ones at any moment.

    IFunction provides methods for tying and untying parameters. Only the
   declared parameters can be
    tied. isFixed() method returns true for a tied parameter. The value of a
   tied parameter is defined
    by its tie and can change in a fit.

    Method addConstraint adds constraints on possible values of a declared
   parameter. A constrained parameter
    is not fixed and can vary within its constraint. Constraints and ties are
   used only in fitting.

    Any non-fitting parameters can be implemented as attributes (class
   IFunction::Attribute).
    An attribute can have one of three types: std::string, int, or double. The
   type is set at construction
    and cannot be changed later. To read or write the attributes there are two
   ways. If the type
    is known the type specific accessors can be used, e.g. asString(), asInt().
   Otherwise the
    IFunction::AttributeVisitor can be used. It provides alternative virtual
   methods to access
    attributes of each type. When creating a function from a string (using
   FunctionFactory::creaeInitialized(...))
    the attributes must be set first, before any fitting parameter, as the
   number and names of the parameters
    can depend on the attributes.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009
    @date 22/12/2010
*/
class MANTID_API_DLL IFunction {
public:
  /**
   * Simple Exception Struct to differentiate validation error from other exceptions.
   */
  struct ValidationException : public std::runtime_error {
  public:
    ValidationException(const std::string &ErrorMsg) : std::runtime_error(ErrorMsg) {}
  };

  /**
   * Simple struct to constain function to evaluate attribute validators that is required in both attributes and
   * visitors.
   */
  struct ValidatorEvaluator {
  public:
    ValidatorEvaluator(){}; // default constructor
    template <typename T1> static void evaluate(T1 &inputData, Mantid::Kernel::IValidator_sptr validator) {
      std::string error;

      if (validator != nullptr) {
        error = validator->isValid(inputData);
      }

      if (error != "") {
        throw IFunction::ValidationException("Set Attribute Error: " + error);
      }
    }
  };

  /**
   * Atribute visitor class. It provides a separate access method
   * for each attribute type. When applied to a particular attribute
   * the appropriate method will be used. The child classes must
   * implement the virtual AttributeVisitor::apply methods. See
   * implementation of Attribute::value() method for an example.
   */
  template <typename T = void> class DLLExport AttributeVisitor : public boost::static_visitor<T> {
  public:
    /// Virtual destructor
    virtual ~AttributeVisitor() = default;
    /// implements static_visitor's operator() for std::string
    T operator()(std::string &str) const { return apply(str); }
    /// implements static_visitor's operator() for double
    T operator()(double &d) const { return apply(d); }
    /// implements static_visitor's operator() for int
    T operator()(int &i) const { return apply(i); }
    /// implements static_visitor's operator() for bool
    T operator()(bool &b) const { return apply(b); }
    /// implements static_visitor's operator() for vector
    T operator()(std::vector<double> &v) const { return apply(v); }

  protected:
    /// Implement this method to access attribute as string
    virtual T apply(std::string &) const = 0;
    /// Implement this method to access attribute as double
    virtual T apply(double &) const = 0;
    /// Implement this method to access attribute as int
    virtual T apply(int &) const = 0;
    /// Implement this method to access attribute as bool
    virtual T apply(bool &) const = 0;
    /// Implement this method to access attribute as vector
    virtual T apply(std::vector<double> &) const = 0;

    /// Evaluates the validator associated with attribute this visitor is to visit.
    template <typename T1> void evaluateValidator(T1 &inputData) const {
      if (m_validator != nullptr) {
        ValidatorEvaluator::evaluate(inputData, m_validator);
      }
    }

    /// Validator against which to evaluate attribute value to set.
    Mantid::Kernel::IValidator_sptr m_validator = Mantid::Kernel::IValidator_sptr();
  };

  /**
   * Const version of AttributeVisitor.
   */
  template <typename T = void> class DLLExport ConstAttributeVisitor : public boost::static_visitor<T> {
  public:
    /// Virtual destructor
    virtual ~ConstAttributeVisitor() = default;
    /// implements static_visitor's operator() for std::string
    T operator()(const std::string &str) const { return apply(str); }
    /// implements static_visitor's operator() for double
    T operator()(const double &d) const { return apply(d); }
    /// implements static_visitor's operator() for int
    T operator()(const int &i) const { return apply(i); }
    /// implements static_visitor's operator() for bool
    T operator()(const bool &b) const { return apply(b); }
    /// implements static_visitor's operator() for vector
    T operator()(const std::vector<double> &v) const { return apply(v); }

  protected:
    /// Implement this mathod to access attribute as string
    virtual T apply(const std::string &str) const = 0;
    /// Implement this mathod to access attribute as double
    virtual T apply(const double &d) const = 0;
    /// Implement this mathod to access attribute as int
    virtual T apply(const int &i) const = 0;
    /// Implement this mathod to access attribute as bool
    virtual T apply(const bool &i) const = 0;
    /// Implement this mathod to access attribute as vector
    virtual T apply(const std::vector<double> &) const = 0;

    /// Evaluates the validator associated with attribute this visitor is to visit.
    template <typename T1> void evaluateValidator(T1 &inputData) const {
      if (m_validator != nullptr) {
        ValidatorEvaluator::evaluate(inputData, m_validator);
      }
    }

    /// Validator against which to evaluate attribute value to set.
    Mantid::Kernel::IValidator_sptr m_validator = Mantid::Kernel::IValidator_sptr();
  };

  /// Attribute is a non-fitting parameter.
  /// It can be one of the types: std::string, int, or double
  /// Examples: file name, polinomial order
  class MANTID_API_DLL Attribute {
  public:
    /// Create empty string attribute
    explicit Attribute() : m_data(std::string()), m_quoteValue(false) {}
    /// Create string attribute
    explicit Attribute(const std::string &str, bool quoteValue = false) : m_data(str), m_quoteValue(quoteValue) {}
    /// Create int attribute
    explicit Attribute(const int &i) : m_data(i), m_quoteValue(false) {}
    /// Create double attribute
    explicit Attribute(const double &d) : m_data(d), m_quoteValue(false) {}
    /// Create bool attribute
    explicit Attribute(const bool &b) : m_data(b), m_quoteValue(false) {}
    /// Create string attribute
    explicit Attribute(const char *c) : m_data(std::string(c)), m_quoteValue(false) {}
    /// Create vector attribute
    explicit Attribute(const std::vector<double> &v) : m_data(v), m_quoteValue(false) {}

    /// Apply an attribute visitor
    template <typename T> T apply(AttributeVisitor<T> &v) { return boost::apply_visitor(v, m_data); }
    /// Apply a const attribute visitor
    template <typename T> T apply(ConstAttributeVisitor<T> &v) const { return boost::apply_visitor(v, m_data); }
    /// Apply a lambda visitor
    template <typename... Ts> void apply(AttributeLambdaVisitor<Ts...> &v) { boost::apply_visitor(v, m_data); }

    /// Set validator to enforce limits on attribute value
    void setValidator(const Kernel::IValidator_sptr &validator) const;
    /// Evaluates the validator associated with this attribute. Returns error as a string.
    void evaluateValidator() const;
    /// Evaluates the validator associated with this attribute with regards to input value. Returns error as a string.
    template <typename T> void evaluateValidator(T &inputData) const {
      if (m_validator != nullptr) {
        ValidatorEvaluator::evaluate(inputData, m_validator);
      }
    }
    /// Return a clone of the attribute validator;
    Kernel::IValidator_sptr getValidator() { return m_validator; }

    /// Returns type of the attribute
    std::string type() const;
    /// Returns the attribute value as a string
    std::string value() const;
    /// Returns string value if attribute is a string, throws exception
    /// otherwise
    std::string asString() const;
    /// Returns a string value that is guarenteed to be quoted for use in places
    /// where the string is used as the displayed value.
    std::string asQuotedString() const;
    /// Returns a string value that is guarenteed to be unquoted.
    std::string asUnquotedString() const;
    /// Returns int value if attribute is a int, throws exception otherwise
    int asInt() const;
    /// Returns double value if attribute is a double, throws exception
    /// otherwise
    double asDouble() const;
    /// Returns bool value if attribute is a bool, throws exception otherwise
    bool asBool() const;
    /// Returns vector<double> if attribute is vector<double>, throws exception otherwise
    std::vector<double> asVector() const;
    /// Check if a string attribute is empty
    bool isEmpty() const;

    /// Sets new value if attribute is a string
    void setString(const std::string &str);
    /// Sets new value if attribute is a double
    void setDouble(const double &);
    /// Sets new value if attribute is a int
    void setInt(const int &);
    /// Sets new value if attribute is a bool
    void setBool(const bool &);
    /// Sets new value if attribute is a vector
    void setVector(const std::vector<double> &);
    // Set value
    template <typename T> void setValue(const T &v) {
      evaluateValidator(v);

      m_data = v;
    }
    /// Set value from a string.
    void fromString(const std::string &str);

  private:
    /// The data holder as boost variant
    mutable boost::variant<std::string, int, double, bool, std::vector<double>> m_data;
    /// Flag indicating if the string value should be returned quoted
    bool m_quoteValue = false;
    /// Associated Validator
    mutable Kernel::IValidator_sptr m_validator;
  };

  /**
   * Atribute validator visitor class. It is used to access
   * m_data of a specificed attribute in order to
   * evaluate the current value of the attribute against
   * the associated validator, for all allowed types.
   */
  template <typename T = void> class DLLExport AttributeValidatorVisitor : public boost::static_visitor<T> {
  public:
    AttributeValidatorVisitor(const IFunction::Attribute *attrToValidate) : m_attrToValidate{attrToValidate} {}

    /// implements static_visitor's operator() for std::string
    T operator()(std::string &str) const { m_attrToValidate->evaluateValidator(str); }
    /// implements static_visitor's operator() for double
    T operator()(double &d) const { m_attrToValidate->evaluateValidator(d); }
    /// implements static_visitor's operator() for int
    T operator()(int &i) const { m_attrToValidate->evaluateValidator(i); }
    /// implements static_visitor's operator() for bool
    T operator()(bool &b) const { m_attrToValidate->evaluateValidator(b); }
    /// implements static_visitor's operator() for vector
    T operator()(std::vector<double> &v) const { m_attrToValidate->evaluateValidator(v); }

  protected:
    const IFunction::Attribute *m_attrToValidate;
  };

  //---------------------------------------------------------//
  /// Constructor
  IFunction();
  /// Virtual destructor
  virtual ~IFunction();
  /// No copying
  IFunction(const IFunction &) = delete;
  /// No copying
  IFunction &operator=(const IFunction &) = delete;
  /// Returns the function's name
  virtual std::string name() const = 0;
  /// Writes itself into a string
  std::string asString() const;
  /// Virtual copy constructor
  virtual std::shared_ptr<IFunction> clone() const;

  /** Registers the usage of the algorithm with the UsageService
   */
  virtual void registerFunctionUsage(bool internal);
  /// Set the workspace.
  /// @param ws :: Shared pointer to a workspace
  virtual void setWorkspace(std::shared_ptr<const Workspace> ws) { UNUSED_ARG(ws); }
  /// Set matrix workspace
  virtual void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                                  double endX);
  /// Iinialize the function
  virtual void initialize() { this->init(); }
  /// Returns an estimate of the number of progress reports a single evaluation
  /// of the function will have. For backwards compatibility default=1
  virtual int64_t estimateNoProgressCalls() const { return 1; }

  /// Attach a progress reporter
  void setProgressReporter(std::shared_ptr<Kernel::ProgressBase> reporter);
  /// Reports progress with an optional message
  void reportProgress(const std::string &msg = "") const;
  /// Returns true if a progress reporter is set & evalaution has been requested
  /// to stop
  bool cancellationRequestReceived() const;

  /// The categories the Fit function belong to.
  /// Categories must be listed as a semi colon separated list.
  /// For example: "General, Muon\\Custom" which adds
  /// a function to the category "General" and the sub-category
  /// "Muon\\Custom"
  virtual const std::string category() const { return "General"; }
  /// Function to return all of the categories that contain this algorithm
  virtual const std::vector<std::string> categories() const;
  /// Function to return the sperator token for the category string. A default
  /// implementation ';' is provided
  virtual const std::string categorySeparator() const { return ";"; }

  /// Evaluates the function for all arguments in the domain.
  /// @param domain :: Provides arguments for the function.
  /// @param values :: A buffer to store the function values. It must be large
  /// enogh to store domain.size() values.
  virtual void function(const FunctionDomain &domain, FunctionValues &values) const = 0;
  /// Derivatives of function with respect to active parameters.
  virtual void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian);

  /* @name Callbacks to perform work at various points other than in the
   * function */
  /// Called at the start of each iteration
  virtual void iterationStarting() {}
  /// Called at the end of an iteration
  virtual void iterationFinished() {}

  /** @name Function parameters */
  //@{
  /// Set i-th parameter
  virtual void setParameter(size_t, const double &value, bool explicitlySet = true) = 0;
  /// Set i-th parameter description
  virtual void setParameterDescription(size_t, const std::string &description) = 0;
  /// Get i-th parameter
  virtual double getParameter(size_t i) const = 0;
  /// Set parameter by name.
  virtual void setParameter(const std::string &name, const double &value, bool explicitlySet = true) = 0;
  /// Set description of parameter by name.
  virtual void setParameterDescription(const std::string &name, const std::string &description) = 0;
  /// Get parameter by name.
  virtual double getParameter(const std::string &name) const = 0;
  /// Check if function has a parameter with this name.
  virtual bool hasParameter(const std::string &name) const = 0;
  /// Total number of parameters
  virtual size_t nParams() const = 0;
  /// Returns the index of parameter name
  virtual size_t parameterIndex(const std::string &name) const = 0;
  /// Returns the name of parameter i
  virtual std::string parameterName(size_t i) const = 0;
  /// Returns the description of parameter i
  virtual std::string parameterDescription(size_t i) const = 0;
  /// Checks if a parameter has been set explicitly
  virtual bool isExplicitlySet(size_t i) const = 0;
  /// Get the fitting error for a parameter
  virtual double getError(size_t i) const = 0;
  /// Get the fitting error for a parameter by name
  virtual double getError(const std::string &name) const = 0;
  /// Set the fitting error for a parameter
  virtual void setError(size_t i, double err) = 0;
  /// Set the fitting error for a parameter by name
  virtual void setError(const std::string &name, double err) = 0;

  /// Check if a parameter i is fixed
  [[nodiscard]] bool isFixed(size_t i) const;
  /// Check if a parameter i is fixed by default (not by user).
  [[nodiscard]] bool isFixedByDefault(size_t i) const;
  /// Removes a parameter i from the list of active
  void fix(size_t i, bool isDefault = false);
  /// Restores a declared parameter i to the active status
  void unfix(size_t i);
  /// Fix a parameter
  void fixParameter(const std::string &name, bool isDefault = false);
  /// Free a parameter
  void unfixParameter(const std::string &name);
  /// Fix all parameters
  void fixAll(bool isDefault = false);
  /// Free all parameters
  void unfixAll();
  /// Free all parameters fixed by default
  void unfixAllDefault();
  /// Fix all active parameters
  void fixAllActive(bool isDefault = false);

  /// Return parameter index from a parameter reference. Usefull for constraints
  /// and ties in composite functions
  virtual size_t getParameterIndex(const ParameterReference &ref) const = 0;
  /// Return a vector with all parameter names
  [[nodiscard]] std::vector<std::string> getParameterNames() const;
  //@}

  /** @name Active parameters */
  //@{
  /// Value of i-th active parameter. Override this method to make fitted
  /// parameters different from the declared
  [[nodiscard]] virtual double activeParameter(size_t i) const;
  /// Set new value of i-th active parameter. Override this method to make
  /// fitted parameters different from the declared
  virtual void setActiveParameter(size_t i, double value);
  /// Returns the name of active parameter i
  [[nodiscard]] virtual std::string nameOfActive(size_t i) const;
  /// Returns the name of active parameter i
  [[nodiscard]] virtual std::string descriptionOfActive(size_t i) const;
  /// Check if an active parameter i is actually active
  [[nodiscard]] bool isActive(size_t i) const;
  //@}

  /** @name Ties */
  //@{
  /// Tie a parameter to other parameters (or a constant)
  virtual void tie(const std::string &parName, const std::string &expr, bool isDefault = false);
  /// Add several ties
  virtual void addTies(const std::string &ties, bool isDefault = false);
  /// Apply the ties
  virtual void applyTies();
  /// Removes the tie off a parameter
  virtual void removeTie(const std::string &parName);
  /// Remove all ties
  virtual void clearTies();
  /// Removes i-th parameter's tie
  virtual bool removeTie(size_t i);
  /// Get the tie of i-th parameter
  virtual ParameterTie *getTie(size_t i) const;
  /// Ignore a tie
  virtual bool ignoreTie(const ParameterTie &) const { return false; }
  /// Put all ties in order in which they will be applied correctly.
  void sortTies(const bool checkOnly = false);
  /// Write a parameter tie to a string
  [[nodiscard]] std::string writeTies() const;
  //@}

  /** @name Constraints */
  //@{
  /// Add a list of conatraints from a string
  virtual void addConstraints(const std::string &str, bool isDefault = false);
  /// Add a constraint to function
  virtual void addConstraint(std::unique_ptr<IConstraint> ic);
  /// Get constraint of i-th parameter
  [[nodiscard]] virtual IConstraint *getConstraint(size_t i) const;
  /// Remove a constraint
  virtual void removeConstraint(const std::string &parName);
  virtual void setConstraintPenaltyFactor(const std::string &parName, const double &c);
  /// Write a parameter constraint to a string
  [[nodiscard]] std::string writeConstraints() const;
  /// Remove all constraints.
  virtual void clearConstraints();
  //@}

  /** @name Attributes */
  //@{
  /// Returns the number of attributes associated with the function
  [[nodiscard]] virtual size_t nAttributes() const;
  /// Returns a list of attribute names
  [[nodiscard]] virtual std::vector<std::string> getAttributeNames() const;
  /// Get name of ith attribute
  [[nodiscard]] virtual std::string attributeName(size_t index) const;
  /// Return a value of attribute attName
  [[nodiscard]] virtual Attribute getAttribute(const std::string &name) const;
  /// Set a value to attribute attName
  virtual void setAttribute(const std::string &name, const Attribute &);
  /// Check if attribute attName exists
  [[nodiscard]] virtual bool hasAttribute(const std::string &name) const;
  /// Set an attribute value
  GNU_DIAG_OFF("maybe-uninitialized")
  template <typename T> void setAttributeValue(const std::string &attName, const T &value) {
    // Since we can't know T and we would rather not create a universal setter
    // copy and replace in-place
    auto attr = getAttribute(attName);
    attr.setValue(value);
    setAttribute(attName, attr);
  }
  GNU_DIAG_ON("maybe-uninitialized")
  void setAttributeValue(const std::string &attName, const char *value);
  void setAttributeValue(const std::string &attName, const std::string &value);
  //@}

  /// Returns the pointer to i-th child function
  [[nodiscard]] virtual std::shared_ptr<IFunction> getFunction(size_t i) const;
  /// Number of child functions
  [[nodiscard]] virtual std::size_t nFunctions() const { return 0; }
  /// Set up the function for a fit.
  virtual void setUpForFit();
  /// Get number of values for a given domain.
  [[nodiscard]] virtual size_t getValuesSize(const FunctionDomain &domain) const;
  /// Get number of domains required by this function
  [[nodiscard]] virtual size_t getNumberDomains() const;
  /// Split this function (if needed) into a list of independent functions.
  [[nodiscard]] virtual std::vector<std::shared_ptr<IFunction>> createEquivalentFunctions() const;
  /// Calculate numerical derivatives
  void calNumericalDeriv(const FunctionDomain &domain, Jacobian &jacobian);
  /// Calculate step size for the given parameter value
  [[nodiscard]] double calculateStepSize(const double parameterValue) const;
  /// Set the covariance matrix
  void setCovarianceMatrix(const std::shared_ptr<Kernel::Matrix<double>> &covar);
  /// Get the covariance matrix
  std::shared_ptr<const Kernel::Matrix<double>> getCovarianceMatrix() const { return m_covar; }
  /// Set the reduced chi^2
  void setReducedChiSquared(double chi2) { m_chiSquared = chi2; }
  /// Get the reduced chi^2
  [[nodiscard]] double getReducedChiSquared() const { return m_chiSquared; }

  /// Set the parallel hint
  void setParallel(bool on) { m_isParallel = on; }
  /// Get the parallel hint
  [[nodiscard]] bool isParallel() const { return m_isParallel; }

  /// Set a function handler
  void setHandler(std::unique_ptr<FunctionHandler> handler);
  /// Return the handler
  [[nodiscard]] FunctionHandler *getHandler() const { return m_handler.get(); }

  /// Describe parameter status in relation to fitting:
  /// Active: Fit varies such parameter directly.
  /// Fixed:  Value doesn't change during fit.
  /// FixedByDefault:  Fixed by default, don't show in ties of
  ///         the output string.
  /// Tied:   Value depends on values of other parameters.
  enum ParameterStatus { Active, Fixed, FixedByDefault, Tied };
  /// Change status of parameter
  virtual void setParameterStatus(size_t i, ParameterStatus status) = 0;
  /// Get status of parameter
  [[nodiscard]] virtual ParameterStatus getParameterStatus(size_t i) const = 0;

  /// Describes the method in which the step size will be calculated:
  /// DEFAULT: Uses the traditional Mantid method of calculating the step size.
  /// SQRT_EPSILON: Uses the square root of epsilon to calculate the step size.
  enum class StepSizeMethod { DEFAULT, SQRT_EPSILON };
  /// Sets the StepSizeMethod to use when calculation the step size
  virtual void setStepSizeMethod(const StepSizeMethod method);

protected:
  /// Function initialization. Declare function parameters in this method.
  virtual void init();
  /// Declare a new parameter
  virtual void declareParameter(const std::string &name, double initValue = 0, const std::string &description = "") = 0;

  /// Convert a value from one unit (inUnit) to unit defined in workspace (ws)
  [[nodiscard]] double convertValue(double value, Kernel::Unit_sptr &outUnit,
                                    const std::shared_ptr<const MatrixWorkspace> &ws, size_t wsIndex) const;

  void convertValue(std::vector<double> &values, Kernel::Unit_sptr &outUnit,
                    const std::shared_ptr<const MatrixWorkspace> &ws, size_t wsIndex) const;

  /// Override to declare function attributes
  virtual void declareAttributes() {}
  /// Override to declare function parameters
  virtual void declareParameters() {}

  /// Declare a single attribute
  void declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue);
  /// Declare a single attribute with validator
  void declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue,
                        const Kernel::IValidator &validator);
  /// Check Attribute to declare does not already exist.
  void checkAttributeName(const std::string &name);
  /// Store an attribute's value
  void storeAttributeValue(const std::string &name, const API::IFunction::Attribute &value);
  /// A read-only ("mutable") attribute can be stored in a const method
  void storeReadOnlyAttribute(const std::string &name, const API::IFunction::Attribute &value) const;
  /// Add a new tie. Derived classes must provide storage for ties
  virtual void addTie(std::unique_ptr<ParameterTie> tie);
  [[nodiscard]] bool hasOrderedTies() const;
  void applyOrderedTies();
  /// Writes itself into a string
  [[nodiscard]] virtual std::string writeToString(const std::string &parentLocalAttributesStr = "") const;

  friend class ParameterTie;
  friend class CompositeFunction;
  friend class FunctionParameterDecorator;
  friend class FunctionGenerator;

  /// Flag to hint that the function is being used in parallel computations
  bool m_isParallel;

  /// Pointer to a function handler
  std::unique_ptr<FunctionHandler> m_handler;

  /// Pointer to the progress handler
  std::shared_ptr<Kernel::ProgressBase> m_progReporter;

private:
  /// The declared attributes
  std::map<std::string, API::IFunction::Attribute> m_attrs;
  /// The covariance matrix of the fitting parameters
  std::shared_ptr<Kernel::Matrix<double>> m_covar;
  /// The chi-squared of the last fit
  double m_chiSquared;
  /// Holds parameter ties
  std::vector<std::unique_ptr<ParameterTie>> m_ties;
  /// Holds the constraints added to function
  std::vector<std::unique_ptr<IConstraint>> m_constraints;
  /// Ties ordered in order of correct application
  std::vector<ParameterTie *> m_orderedTies;
  /// whether the function usage has been registered
  bool m_isRegistered{false};
  /// The function used to calculate the step size
  std::function<double(const double)> m_stepSizeFunction;
};

/// shared pointer to the function base class
using IFunction_sptr = std::shared_ptr<IFunction>;
/// shared pointer to the function base class (const version)
using IFunction_const_sptr = std::shared_ptr<const IFunction>;

/**
 * Classes inherited from FunctionHandler will handle the function.
 * The intended purpose is to help with displaying nested composite
 * functions in a tree view. This way a display handler shows only
 * single function and there is no need to duplicate the function tree
 * structure.
 */
class FunctionHandler {
public:
  /// Constructor
  FunctionHandler(IFunction_sptr fun) : m_fun(std::move(fun)) {}
  /// Delete copy constructor and copy assignment operator
  FunctionHandler(const FunctionHandler &) = delete;
  FunctionHandler &operator=(const FunctionHandler &) = delete;
  /// Virtual destructor
  virtual ~FunctionHandler() = default;
  /// abstract init method. It is called after setting handler to the function
  virtual void init() = 0;
  /// Return the handled function
  IFunction_sptr function() const { return m_fun; }

protected:
  IFunction_sptr m_fun; ///< pointer to the handled function
};

} // namespace API
} // namespace Mantid
