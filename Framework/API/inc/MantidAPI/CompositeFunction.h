// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_COMPOSITEFUNCTION_H_
#define MANTID_API_COMPOSITEFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"
#include "MantidAPI/Jacobian.h"

#include <map>

namespace Mantid {
namespace API {
/** A composite function is a function containing other functions. It combines
   values
    calculated by the member function using an operation. The default operation
   is summation (+).
    Composite functions do not have their own parameters, they use parameters of
   the member functions.
    Functions are added to a composite functions with addFunction method and can
   be retrieved with
    getFinction(i) method. Function indices are defined by the order they are
   added. Parameter names
    are formed from the member function's index and its parameter name:
   f[index].[name]. For example,
    name "f0.Sigma" would be given to the "Sigma" parameter of a Gaussian added
   first to the composite
    function. If a member function is a composite function itself the same
   principle applies: 'f[index].'
    is prepended to a name, e.g. "f0.f1.Sigma".

    The default implementation expects its member to use the same type of
   FunctionDomain. The domain
    passed to the function(...) method is used to evaluate all member functions.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 20/10/2009
*/
class MANTID_API_DLL CompositeFunction : public virtual IFunction {
public:
  /// Default constructor
  CompositeFunction();

  /* Overriden methods */

  /// Returns the function's name
  std::string name() const override { return "CompositeFunction"; }
  /// Sets the workspace for each member function
  void setWorkspace(boost::shared_ptr<const Workspace> ws) override;
  /// Set matrix workspace
  void
  setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,
                     size_t wi, double startX, double endX) override;

  /// Function you want to fit to.
  void function(const FunctionDomain &domain,
                FunctionValues &values) const override;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;

  /// Set i-th parameter
  void setParameter(size_t, const double &value,
                    bool explicitlySet = true) override;
  /// Set i-th parameter description
  void setParameterDescription(size_t, const std::string &description) override;
  /// Get i-th parameter
  double getParameter(size_t i) const override;
  /// Set parameter by name.
  void setParameter(const std::string &name, const double &value,
                    bool explicitlySet = true) override;
  /// Set description of parameter by name.
  void setParameterDescription(const std::string &name,
                               const std::string &description) override;
  /// Get parameter by name.
  double getParameter(const std::string &name) const override;
  /// Check if function has a parameter with this name.
  bool hasParameter(const std::string &name) const override;
  /// Total number of parameters
  size_t nParams() const override;
  /// Returns the index of parameter name
  size_t parameterIndex(const std::string &name) const override;
  /// Returns the name of parameter i
  std::string parameterName(size_t i) const override;
  /// Returns the description of parameter i
  std::string parameterDescription(size_t i) const override;
  /// Checks if a parameter has been set explicitly
  bool isExplicitlySet(size_t i) const override;
  /// Get the fitting error for a parameter
  double getError(size_t i) const override;
  /// Set the fitting error for a parameter
  void setError(size_t i, double err) override;

  /// Value of i-th active parameter. Override this method to make fitted
  /// parameters different from the declared
  double activeParameter(size_t i) const override;
  /// Set new value of i-th active parameter. Override this method to make
  /// fitted parameters different from the declared
  void setActiveParameter(size_t i, double value) override;
  /// Update parameters after a fitting iteration
  void updateActive(const double *in);
  /// Returns the name of active parameter i
  std::string nameOfActive(size_t i) const override;
  /// Returns the name of active parameter i
  std::string descriptionOfActive(size_t i) const override;

  /// Return parameter index from a parameter reference.
  size_t getParameterIndex(const ParameterReference &ref) const override;
  /// Get the containing function
  IFunction_sptr getContainingFunction(const ParameterReference &ref) const;

  /// Apply the ties
  void applyTies() override;
  /// Remove all ties
  void clearTies() override;
  // Unhide base class function: removeTie(string). Avoids Intel compiler
  // warning
  using IFunction::removeTie;
  /// Removes i-th parameter's tie
  bool removeTie(size_t i) override;
  /// Get the tie of i-th parameter
  ParameterTie *getTie(size_t i) const override;

  /// Get constraint of i-th parameter
  IConstraint *getConstraint(size_t i) const override;
  /// Prepare function for a fit
  void setUpForFit() override;
  /// Remove a constraint
  void removeConstraint(const std::string &parName) override;
  /// Get number of domains required by this function
  size_t getNumberDomains() const override;
  /// Split this function (if needed) into a list of independent functions.
  std::vector<boost::shared_ptr<IFunction>>
  createEquivalentFunctions() const override;
  /// Returns the pointer to i-th function
  IFunction_sptr getFunction(std::size_t i) const override;
  /// Number of functions
  std::size_t nFunctions() const override { return m_functions.size(); }

  /* CompositeFunction own methods */

  /// Add a function at the back of the internal function list
  virtual size_t addFunction(IFunction_sptr f);
  /// Remove a function
  void removeFunction(size_t i);
  /// Replace a function
  void replaceFunction(size_t i, IFunction_sptr f);
  /// Replace a function
  void replaceFunctionPtr(const IFunction_sptr f_old, IFunction_sptr f_new);
  /// Get the function index
  std::size_t functionIndex(std::size_t i) const;
  /// Returns the index of parameter i as it declared in its function
  size_t parameterLocalIndex(size_t i, bool recursive = false) const;
  /// Returns the name of parameter i as it declared in its function
  std::string parameterLocalName(size_t i, bool recursive = false) const;
  /// Check the function.
  void checkFunction();
  /// Remove all member functions
  void clear();

  /// Returns the number of attributes associated with the function
  virtual size_t nLocalAttributes() const { return 0; }
  /// Returns a list of attribute names
  virtual std::vector<std::string> getLocalAttributeNames() const {
    return std::vector<std::string>();
  }
  /// Return a value of attribute attName
  virtual Attribute getLocalAttribute(size_t i,
                                      const std::string &attName) const {
    (void)i;
    throw std::invalid_argument("Attribute " + attName +
                                " not found in function " + this->name());
  }
  /// Set a value to attribute attName
  virtual void setLocalAttribute(size_t i, const std::string &attName,
                                 const Attribute &) {
    (void)i;
    throw std::invalid_argument("Attribute " + attName +
                                " not found in function " + this->name());
  }
  /// Check if attribute attName exists
  virtual bool hasLocalAttribute(const std::string &) const { return false; }
  template <typename T>
  void setLocalAttributeValue(size_t i, const std::string &attName,
                              const T &value) {
    setLocalAttribute(i, attName, Attribute(value));
  }
  void setLocalAttributeValue(size_t i, const std::string &attName,
                              const char *value) {
    setLocalAttribute(i, attName, Attribute(std::string(value)));
  }
  /// Change status of parameter
  void setParameterStatus(size_t i, ParameterStatus status) override;
  /// Get status of parameter
  ParameterStatus getParameterStatus(size_t i) const override;

protected:
  /// Function initialization. Declare function parameters in this method.
  void init() override;
  /// Declare a new parameter
  void declareParameter(const std::string &name, double initValue = 0,
                        const std::string &description = "") override;
  /// Writes itself into a string
  std::string writeToString(
      const std::string &parentLocalAttributesStr = "") const override;

  size_t paramOffset(size_t i) const { return m_paramOffsets[i]; }

private:
  /// Extract function index and parameter name from a variable name
  static void parseName(const std::string &varName, size_t &index,
                        std::string &name);

  /// Pointers to the included funtions
  std::vector<IFunction_sptr> m_functions;
  /// Individual function parameter offsets (function index in m_functions)
  /// e.g. m_functions[i]->parameter(m_paramOffsets[i]+1) gives second declared
  /// parameter of i-th function
  std::vector<size_t> m_paramOffsets;
  /// Keeps the function index for each declared parameter  (parameter declared
  /// index)
  std::vector<size_t> m_IFunction;
  /// Total number of parameters
  size_t m_nParams;
  /// Function counter to be used in nextConstraint
  mutable size_t m_iConstraintFunction;
};

/// shared pointer to the composite function base class
using CompositeFunction_sptr = boost::shared_ptr<CompositeFunction>;
/// shared pointer to the composite function base class (const version)
using CompositeFunction_const_sptr = boost::shared_ptr<const CompositeFunction>;

/** A Jacobian for individual functions
 */
class PartialJacobian : public Jacobian {
  Jacobian *m_J; ///< pointer to the overall Jacobian
  size_t m_iY0;  ///< fitting data index offset in the overall Jacobian for a
  /// particular function
  size_t m_iP0; ///< parameter index offset in the overall Jacobian for a
  /// particular function
public:
  /** Constructor
   * @param J :: A pointer to the overall Jacobian
   * @param iP0 :: The parameter index (declared) offset for a particular
   * function
   */
  PartialJacobian(Jacobian *J, size_t iP0)
      : m_J(J), m_iY0(0), m_iP0(iP0) //,m_iaP0(iap0)
  {}
  /** Constructor
   * @param J :: A pointer to the overall Jacobian
   * @param iY0 :: The data index offset for a particular function
   * @param iP0 :: The parameter index offset for a particular function
   */
  PartialJacobian(Jacobian *J, size_t iY0, size_t iP0)
      : m_J(J), m_iY0(iY0), m_iP0(iP0) {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   * @param value :: The derivative value
   */
  void set(size_t iY, size_t iP, double value) override {
    m_J->set(m_iY0 + iY, m_iP0 + iP, value);
  }
  /**
   * Overridden Jacobian::get(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   */
  double get(size_t iY, size_t iP) override {
    return m_J->get(m_iY0 + iY, m_iP0 + iP);
  }
  /** Zero all matrix elements.
   */
  void zero() override {
    throw Kernel::Exception::NotImplementedError(
        "zero() is not implemented for PartialJacobian");
  }
  /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
   *   @param value :: Value to add
   *   @param iP :: The index of an active parameter.
   */
  void addNumberToColumn(const double &value, const size_t &iP) override {
    m_J->addNumberToColumn(value, m_iP0 + iP);
  }
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COMPOSITEFUNCTION_H_*/
