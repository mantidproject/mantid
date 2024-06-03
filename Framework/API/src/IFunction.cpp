// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IFunction.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UsageService.h"

#include <boost/lexical_cast.hpp>

#include "MantidKernel/StringTokenizer.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <utility>

namespace {

constexpr double EPSILON = std::numeric_limits<double>::epsilon();
constexpr double MIN_DOUBLE = std::numeric_limits<double>::min();
constexpr double STEP_PERCENTAGE = 0.001;

const auto defaultStepSize = [](const double parameterValue) -> double {
  return fabs(parameterValue) < 100.0 * MIN_DOUBLE / STEP_PERCENTAGE ? 100.0 * EPSILON
                                                                     : parameterValue * STEP_PERCENTAGE;
};

const auto sqrtEpsilonStepSize = [](const double parameterValue) -> double {
  return fabs(parameterValue) < 1 ? sqrt(EPSILON) : parameterValue * sqrt(EPSILON);
};

} // namespace

namespace Mantid::API {
using namespace Geometry;

namespace {
/// static logger
Kernel::Logger g_log("IFunction");

/// Struct that helps sort ties in correct order of application.
struct TieNode {
  // Index of the tied parameter
  size_t left;
  // Indices of parameters on the right-hand-side of the expression
  std::vector<size_t> right;
  // This tie must be applied before the other if the RHS of the other
  // contains this (left) parameter.
  bool operator<(TieNode const &other) const {
    return std::find(other.right.begin(), other.right.end(), left) != other.right.end();
  }
};
const std::vector<std::string> EXCLUDEUSAGE = {"CompositeFunction"};
} // namespace

/**
 * Constructor
 */
IFunction ::IFunction()
    : m_isParallel(false), m_handler(nullptr), m_chiSquared(0.0), m_stepSizeFunction(defaultStepSize) {}

/**
 * Destructor
 */
IFunction::~IFunction() { m_attrs.clear(); }

/**
Registers the usage of the function with the UsageService
 */
void IFunction::registerFunctionUsage(bool internal) {
  if (!Kernel::UsageService::Instance().isEnabled()) {
    return;
  }
  if (std::find(EXCLUDEUSAGE.cbegin(), EXCLUDEUSAGE.cend(), name()) == EXCLUDEUSAGE.cend() && !m_isRegistered) {
    m_isRegistered = true;
    Kernel::UsageService::Instance().registerFeatureUsage(Kernel::FeatureType::Function, name(), internal);
  }
}
/**
 * Virtual copy constructor
 */
std::shared_ptr<IFunction> IFunction::clone() const {
  auto clonedFunction = FunctionFactory::Instance().createInitialized(this->asString());
  for (size_t i = 0; i < this->nParams(); i++) {
    double error = this->getError(i);
    clonedFunction->setError(i, error);
  }
  return clonedFunction;
}

/**
 * Attach a progress reporter
 * @param reporter :: A pointer to a progress reporter that can be called during
 * function evaluation
 */
void IFunction::setProgressReporter(std::shared_ptr<Kernel::ProgressBase> reporter) {
  m_progReporter = std::move(reporter);
  m_progReporter->setNotifyStep(0.01);
}

/**
 * If a reporter object is set, reports progress with an optional message
 * @param msg :: A message to display (default = "")
 */
void IFunction::reportProgress(const std::string &msg) const {
  if (m_progReporter) {
    const_cast<Kernel::ProgressBase *>(m_progReporter.get())->report(msg);
  }
}

/**
 *
 * @returns true if a progress reporter is set & evalaution has been requested
 *to stop
 */
bool IFunction::cancellationRequestReceived() const {
  if (m_progReporter)
    return m_progReporter->hasCancellationBeenRequested();
  else
    return false;
}

/** Base class implementation calculates the derivatives numerically.
 * @param domain :: The domain of the function
 * @param jacobian :: A Jacobian matrix. It is expected to have dimensions of
 * domain.size() by nParams().
 */
void IFunction::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) { calNumericalDeriv(domain, jacobian); }

/** Check if an active parameter i is actually active
 * @param i :: Index of a parameter.
 */
bool IFunction::isActive(size_t i) const { return getParameterStatus(i) == Active; }

/**
 * Query if the parameter is fixed
 * @param i :: The index of a declared parameter
 * @return true if parameter i is fixed
 */
bool IFunction::isFixed(size_t i) const {
  auto status = getParameterStatus(i);
  return status == Fixed || status == FixedByDefault;
}

/// Check if a parameter i is fixed by default (not by user).
/// @param i :: The index of a parameter
/// @return true if parameter i is fixed by default
bool IFunction::isFixedByDefault(size_t i) const { return getParameterStatus(i) == FixedByDefault; }

/// This method doesn't create a tie
/// @param i :: A declared parameter index to be fixed
/// @param isDefault :: If true fix it by default
///
void IFunction::fix(size_t i, bool isDefault) {
  auto status = getParameterStatus(i);
  if (status == Tied) {
    throw std::runtime_error("Cannot fix parameter " + std::to_string(i) + " (" + parameterName(i) +
                             "): it has a tie.");
  }
  if (isDefault) {
    setParameterStatus(i, FixedByDefault);
  } else {
    setParameterStatus(i, Fixed);
  }
}

/** Makes a parameter active again. It doesn't change the parameter's tie.
 * @param i :: A declared parameter index to be restored to active
 */
void IFunction::unfix(size_t i) {
  auto status = getParameterStatus(i);
  if (status == Tied) {
    throw std::runtime_error("Cannot unfix parameter " + std::to_string(i) + " (" + parameterName(i) +
                             "): it has a tie.");
  }
  setParameterStatus(i, Active);
}

/**
 * Ties a parameter to other parameters
 * @param parName :: The name of the parameter to tie.
 * @param expr :: A math expression
 * @param isDefault :: Flag to mark as default the value of an object associated
 * with this reference: a tie or a constraint.
 */
void IFunction::tie(const std::string &parName, const std::string &expr, bool isDefault) {
  auto ti = std::make_unique<ParameterTie>(this, parName, expr, isDefault);
  if (!isDefault && ti->isConstant()) {
    setParameter(parName, ti->eval());
    fix(getParameterIndex(*ti));
  } else {
    addTie(std::move(ti));
  }
}

/**
 * Add ties to the function.
 * @param ties :: Comma-separated list of name=value pairs where name is a
 *parameter name and value
 *  is a math expression tying the parameter to other parameters or a constant.
 * @param isDefault :: Flag to mark as default the value of an object associated
 *with this reference: a tie or a constraint.
 *
 */
void IFunction::addTies(const std::string &ties, bool isDefault) {
  Expression list;
  list.parse(ties);
  list.toList();
  for (const auto &t : list) {
    if (t.name() == "=" && t.size() >= 2) {
      size_t n = t.size() - 1;
      const std::string value = t[n].str();
      for (size_t i = n; i != 0;) {
        --i;
        this->tie(t[i].name(), value, isDefault);
      }
    }
  }
  applyTies();
}

/** Removes the tie off a parameter. The parameter becomes active
 * This method can be used when constructing and editing the IFunction in a GUI
 * @param parName :: The name of the parameter which ties will be removed.
 */
void IFunction::removeTie(const std::string &parName) {
  size_t i = parameterIndex(parName);
  this->removeTie(i);
}

/// Write all parameter ties owned by this function to a string
/// @return A tie string for the parameter.
std::string IFunction::writeTies() const {
  std::ostringstream tieStream;
  bool first = true;
  for (auto &tie : m_ties) {
    if (tie->isDefault())
      continue;
    if (!first) {
      tieStream << ',';
    } else {
      first = false;
    }
    tieStream << tie->asString(this);
  }
  return tieStream.str();
}

/**
 * Attaches a tie to this ParamFunction. The attached tie is owned by the
 * ParamFunction.
 * @param tie :: A pointer to a new tie
 */
void IFunction::addTie(std::unique_ptr<ParameterTie> tie) {
  auto iPar = getParameterIndex(*tie);
  auto it =
      std::find_if(m_ties.begin(), m_ties.end(), [&](const auto &m_tie) { return getParameterIndex(*m_tie) == iPar; });
  const auto oldTie = getTie(iPar);

  if (it != m_ties.end()) {
    *it = std::move(tie);
  } else {
    m_ties.emplace_back(std::move(tie));
    setParameterStatus(iPar, Tied);
  }

  try {
    // sortTies checks for circular and self ties
    sortTies(true);
  } catch (std::runtime_error &) {
    // revert / remove tie if invalid
    if (oldTie) {
      const auto oldTieStr = oldTie->asString();
      const auto oldExp = oldTieStr.substr(oldTieStr.find("=") + 1);
      *it = std::make_unique<ParameterTie>(this, parameterName(iPar), oldExp);
    } else {
      removeTie(iPar);
    }
    throw;
  }
}

bool IFunction::hasOrderedTies() const { return !m_orderedTies.empty(); }

void IFunction::applyOrderedTies() {
  for (auto &&tie : m_orderedTies) {
    tie->eval();
  }
}

/**
 * Apply the ties.
 */
void IFunction::applyTies() {
  if (hasOrderedTies()) {
    applyOrderedTies();
  } else {
    for (auto &tie : m_ties) {
      tie->eval();
    }
  }
}

/**
 * Used to find ParameterTie for a parameter i
 */
class ReferenceEqual {
  /// The function that has the tie
  const IFunction &m_fun;
  /// index to find
  const size_t m_i;

public:
  /// Constructor
  explicit ReferenceEqual(const IFunction &fun, size_t i) : m_fun(fun), m_i(i) {}
  /// Bracket operator
  /// @param p :: the element you are looking for
  /// @return True if found
  template <class T> bool operator()(const std::unique_ptr<T> &p) { return m_fun.getParameterIndex(*p) == m_i; }
};

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i :: The index of the tied parameter.
 * @return True if successfull
 */
bool IFunction::removeTie(size_t i) {
  if (i >= nParams()) {
    throw std::out_of_range("Function parameter index out of range.");
  }
  auto it = std::find_if(m_ties.begin(), m_ties.end(), ReferenceEqual(*this, i));
  if (it != m_ties.end()) {
    m_ties.erase(it);
    setParameterStatus(i, Active);
    return true;
  }
  unfix(i);
  return false;
}

/** Get tie of parameter number i
 * @param i :: The index of a declared parameter.
 * @return A pointer to the tie
 */
ParameterTie *IFunction::getTie(size_t i) const {
  auto it = std::find_if(m_ties.cbegin(), m_ties.cend(), ReferenceEqual(*this, i));
  if (it != m_ties.cend()) {
    return it->get();
  }
  return nullptr;
}

/** Remove all ties
 */
void IFunction::clearTies() {
  for (size_t i = 0; i < nParams(); ++i) {
    setParameterStatus(i, Active);
  }
  m_ties.clear();
}

/** Add a constraint
 *  @param ic :: Pointer to a constraint.
 */
void IFunction::addConstraint(std::unique_ptr<IConstraint> ic) {
  size_t iPar = ic->parameterIndex();
  auto it = std::find_if(m_constraints.begin(), m_constraints.end(),
                         [&iPar](const auto &constraint) { return constraint->parameterIndex() == iPar; });

  if (it != m_constraints.end()) {
    *it = std::move(ic);
  } else {
    m_constraints.emplace_back(std::move(ic));
  }
}

/** Get constraint of parameter number i
 * @param i :: The index of a declared parameter.
 * @return A pointer to the constraint or NULL
 */
IConstraint *IFunction::getConstraint(size_t i) const {
  auto it = std::find_if(m_constraints.cbegin(), m_constraints.cend(), ReferenceEqual(*this, i));
  if (it != m_constraints.cend()) {
    return it->get();
  }
  return nullptr;
}

/** Remove a constraint
 * @param parName :: The name of a parameter which constarint to remove.
 */
void IFunction::removeConstraint(const std::string &parName) {
  size_t iPar = parameterIndex(parName);
  const auto it = std::find_if(m_constraints.cbegin(), m_constraints.cend(),
                               [&iPar](const auto &constraint) { return iPar == constraint->getLocalIndex(); });
  if (it != m_constraints.cend()) {
    m_constraints.erase(it);
  }
}

/** Set a constraint penalty
 * @param parName :: The name of a constraint
 * @param c :: The penalty
 */
void IFunction::setConstraintPenaltyFactor(const std::string &parName, const double &c) {
  size_t iPar = parameterIndex(parName);
  const auto it = std::find_if(m_constraints.cbegin(), m_constraints.cend(),
                               [&iPar](const auto &constraint) { return iPar == constraint->getLocalIndex(); });

  if (it != m_constraints.cend()) {
    (*it)->setPenaltyFactor(c);
  } else {
    g_log.warning() << parName << " does not have constraint so setConstraintPenaltyFactor failed"
                    << "\n";
  }
}

/// Remove all constraints.
void IFunction::clearConstraints() { m_constraints.clear(); }

void IFunction::setUpForFit() {
  for (auto &constraint : m_constraints) {
    constraint->setParamToSatisfyConstraint();
  }
}

/// Write all parameter constraints owned by this function to a string
/// @return A constraint string for the parameter.
std::string IFunction::writeConstraints() const {
  std::ostringstream stream;
  bool first = true;
  for (const auto &constrint : m_constraints) {
    if (constrint->isDefault())
      continue;
    if (!first) {
      stream << ',';
    } else {
      first = false;
    }
    stream << constrint->asString();
  }
  return stream.str();
}

/**
 * Writes a string that can be used in FunctionFunctory to create a copy of this
 * IFunction
 * @return string representation of the function
 */
std::string IFunction::asString() const { return writeToString(); }

/**
 * Writes this function into a string.
 * @param parentLocalAttributesStr :: A preformatted string with local
 * attributes of a parent composite function. Can be passed in by a
 * CompositeFunction (eg MultiDomainFunction).
 * @return string representation of the function
 */
std::string IFunction::writeToString(const std::string &parentLocalAttributesStr) const {
  std::ostringstream ostr;
  ostr << "name=" << this->name();
  // print the attributes
  std::vector<std::string> attr = this->getAttributeNames();
  for (const auto &attName : attr) {
    std::string attValue = this->getAttribute(attName).value();
    if (!attValue.empty() && attValue != "\"\"") {
      ostr << ',' << attName << '=' << attValue;
    }
  }
  std::vector<std::string> ties;
  // print the parameters
  for (size_t i = 0; i < nParams(); i++) {
    std::ostringstream paramOut;
    paramOut << parameterName(i) << '=' << getParameter(i);
    ostr << ',' << paramOut.str();
    // Output non-default ties only.
    if (getParameterStatus(i) == Fixed) {
      ties.emplace_back(paramOut.str());
    }
  }

  // collect non-default constraints
  std::string constraints = writeConstraints();
  // print constraints
  if (!constraints.empty()) {
    ostr << ",constraints=(" << constraints << ")";
  }

  // collect the non-default ties
  auto tiesString = writeTies();
  if (!tiesString.empty()) {
    ties.emplace_back(tiesString);
  }
  // print the ties
  if (!ties.empty()) {
    ostr << ",ties=(" << Kernel::Strings::join(ties.begin(), ties.end(), ",") << ")";
  }
  // "local" attributes of a parent composite function
  ostr << parentLocalAttributesStr;
  return ostr.str();
}

/** Add a list of constraints from a string
 * @param str :: A comma-separated list of constraint expressions.
 * @param isDefault :: Flag to mark as default the value of an object associated
 *with this reference.
 *
 */
void IFunction::addConstraints(const std::string &str, bool isDefault) {
  Expression list;
  list.parse(str);
  list.toList();
  for (auto it = list.begin(); it != list.end(); ++it) {
    auto expr = (*it);
    if (expr.terms()[0].str().compare("penalty") == 0) {
      continue;
    }
    if ((it + 1) != list.end()) {
      auto next_expr = *(it + 1);
      if (next_expr.terms()[0].str().compare("penalty") == 0) {
        auto c = std::unique_ptr<IConstraint>(ConstraintFactory::Instance().createInitialized(this, expr, isDefault));
        double penalty_factor = std::stof(next_expr.terms()[1].str(), NULL);
        c->setPenaltyFactor(penalty_factor);
        this->addConstraint(std::move(c));
      } else {
        auto c = std::unique_ptr<IConstraint>(ConstraintFactory::Instance().createInitialized(this, expr, isDefault));
        this->addConstraint(std::move(c));
      }
    } else {
      auto c = std::unique_ptr<IConstraint>(ConstraintFactory::Instance().createInitialized(this, expr, isDefault));
      this->addConstraint(std::move(c));
    }
  }
}

/**
 * Return a vector with all parameter names.
 */
std::vector<std::string> IFunction::getParameterNames() const {
  std::vector<std::string> out;
  for (size_t i = 0; i < nParams(); ++i) {
    out.emplace_back(parameterName(i));
  }
  return out;
}

/** Set a function handler
 * @param handler :: A new handler
 */
void IFunction::setHandler(std::unique_ptr<FunctionHandler> handler) {
  if (handler && handler->function().get() != this) {
    throw std::runtime_error("Function handler points to a different function");
  }

  m_handler = std::move(handler);
  m_handler->init();
}

/// Function to return all of the categories that contain this function
const std::vector<std::string> IFunction::categories() const {
  Mantid::Kernel::StringTokenizer tokenizer(category(), categorySeparator(),
                                            Mantid::Kernel::StringTokenizer::TOK_TRIM |
                                                Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  return tokenizer.asVector();
}

/**
 * Operator <<
 * @param ostr :: The output stream
 * @param f :: The IFunction
 */
std::ostream &operator<<(std::ostream &ostr, const IFunction &f) {
  ostr << f.asString();
  return ostr;
}

namespace {
/**
 * Const attribute visitor returning the type of the attribute
 */
class AttType : public IFunction::ConstAttributeVisitor<std::string> {
protected:
  /// Apply if string
  std::string apply(const std::string & /*str*/) const override { return "std::string"; }
  /// Apply if int
  std::string apply(const int & /*i*/) const override { return "int"; }
  /// Apply if double
  std::string apply(const double & /*d*/) const override { return "double"; }
  /// Apply if bool
  std::string apply(const bool & /*i*/) const override { return "bool"; }
  /// Apply if vector
  std::string apply(const std::vector<double> & /*unused*/) const override { return "std::vector<double>"; }
};
} // namespace

std::string IFunction::Attribute::type() const {
  AttType tmp;
  return apply(tmp);
}

namespace {
/**
 * Const attribute visitor returning the value of the attribute as a string
 */
class AttValue : public IFunction::ConstAttributeVisitor<std::string> {
public:
  explicit AttValue(bool quoteString = false)
      : IFunction::ConstAttributeVisitor<std::string>(), m_quoteString(quoteString) {}

protected:
  /// Apply if string
  std::string apply(const std::string &str) const override {
    return (m_quoteString) ? std::string("\"" + str + "\"") : str;
  }
  /// Apply if int
  std::string apply(const int &i) const override { return std::to_string(i); }
  /// Apply if double
  std::string apply(const double &d) const override { return boost::lexical_cast<std::string>(d); }
  /// Apply if bool
  std::string apply(const bool &b) const override { return b ? "true" : "false"; }
  /// Apply if vector
  std::string apply(const std::vector<double> &v) const override {
    std::string res = "(";
    if (!v.empty()) {
      for (size_t i = 0; i < v.size() - 1; ++i) {
        res += boost::lexical_cast<std::string>(v[i]) + ",";
      }
      res += boost::lexical_cast<std::string>(v.back());
    }
    res += ")";
    return res;
  }

private:
  /// Flag to quote a string value returned
  bool m_quoteString;
};
} // namespace

std::string IFunction::Attribute::value() const {
  AttValue tmp(m_quoteValue);
  return apply(tmp);
}

/**
 * Return the attribute as a string if it is a string.
 */
std::string IFunction::Attribute::asString() const {
  if (m_quoteValue)
    return asQuotedString();

  try {
    return boost::get<std::string>(m_data);
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as string");
  }
}

/**
 * Return the attribute as a quoted string if it is a string.
 */
std::string IFunction::Attribute::asQuotedString() const {
  std::string attr;

  try {
    attr = boost::get<std::string>(m_data);
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as string");
  }

  if (attr.empty())
    return "\"\"";

  std::string quoted(attr);
  if (*(attr.begin()) != '\"')
    quoted = "\"" + attr;
  if (*(quoted.end() - 1) != '\"')
    quoted += "\"";

  return quoted;
}

/**
 * Return the attribute as an unquoted string if it is a string.
 */
std::string IFunction::Attribute::asUnquotedString() const {
  std::string attr;

  try {
    attr = boost::get<std::string>(m_data);
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as string");
  }
  std::string unquoted(attr);
  if (attr.empty())
    return "";
  if (*(attr.begin()) == '\"')
    unquoted = std::string(attr.begin() + 1, attr.end() - 1);
  if (*(unquoted.end() - 1) == '\"')
    unquoted.resize(unquoted.size() - 1);

  return unquoted;
}

/**
 * Return the attribute as an int if it is a int.
 */
int IFunction::Attribute::asInt() const {
  try {
    return boost::get<int>(m_data);
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as int");
  }
}

/**
 * Return the attribute as a double if it is a double.
 */
double IFunction::Attribute::asDouble() const {
  try {
    return boost::get<double>(m_data);
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as double");
  }
}

/**
 * Return the attribute as a bool if it is a bool.
 */
bool IFunction::Attribute::asBool() const {
  try {
    return boost::get<bool>(m_data);
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as bool");
  }
}

/**
 * Return the attribute as a bool if it is a vector.
 */
std::vector<double> IFunction::Attribute::asVector() const {
  try {
    return boost::get<std::vector<double>>(m_data);
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as vector");
  }
}

/** Sets new value if attribute is a string. If the type is wrong
 * throws an exception
 * @param str :: The new value
 */
void IFunction::Attribute::setString(const std::string &str) {
  evaluateValidator(str);

  try {
    boost::get<std::string>(m_data) = str;
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as string");
  }
}

/** Sets new value if attribute is a double. If the type is wrong
 * throws an exception
 * @param d :: The new value
 */
void IFunction::Attribute::setDouble(const double &d) {
  evaluateValidator(d);

  try {
    boost::get<double>(m_data) = d;
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as double");
  }
}

/** Sets new value if attribute is an int. If the type is wrong
 * throws an exception
 * @param i :: The new value
 */
void IFunction::Attribute::setInt(const int &i) {
  evaluateValidator(i);

  try {
    boost::get<int>(m_data) = i;
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as int");
  }
}

/** Sets new value if attribute is an bool. If the type is wrong
 * throws an exception
 * @param b :: The new value
 */
void IFunction::Attribute::setBool(const bool &b) {
  evaluateValidator(b);

  try {
    boost::get<bool>(m_data) = b;
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as bool");
  }
}

/**
 * Sets new value if attribute is a vector. If the type is wrong
 * throws an exception
 * @param v :: The new value
 */
void IFunction::Attribute::setVector(const std::vector<double> &v) {
  evaluateValidator(v);

  try {
    auto &data = boost::get<std::vector<double>>(m_data);
    data.assign(v.begin(), v.end());
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() +
                             " attribute "
                             "as vector");
  }
}

/// Check if a string attribute is empty
bool IFunction::Attribute::isEmpty() const {
  try {
    return boost::get<std::string>(m_data).empty();
  } catch (...) {
    throw std::runtime_error("Trying to access a " + type() + " attribute as string");
  }
}

namespace {
/**
 * Attribute visitor setting new value to an attribute
 */
class SetValue : public IFunction::AttributeVisitor<> {
public:
  /**
   * Constructor
   * @param value :: The value to set
   * @param validator :: Associated validator
   */
  explicit SetValue(std::string value, Mantid::Kernel::IValidator_sptr validator = Mantid::Kernel::IValidator_sptr())
      : m_value(std::move(value)), m_validator(validator) {}

protected:
  /// Apply if string
  void apply(std::string &str) const override {
    evaluateValidator(m_value);
    str = m_value;
  }
  /// Apply if int
  void apply(int &i) const override {
    int tempi = 0;

    std::istringstream istr(m_value + " ");
    istr >> tempi;
    if (!istr.good())
      throw std::invalid_argument("Failed to set int attribute "
                                  "from string " +
                                  m_value);

    evaluateValidator(tempi);
    i = tempi;
  }
  /// Apply if double
  void apply(double &d) const override {
    double tempd = 0;

    std::istringstream istr(m_value + " ");
    istr >> tempd;
    if (!istr.good())
      throw std::invalid_argument("Failed to set double attribute "
                                  "from string " +
                                  m_value);

    evaluateValidator(tempd);
    d = tempd;
  }
  /// Apply if bool
  void apply(bool &b) const override {
    bool tempb = false;

    tempb = (m_value == "true" || m_value == "TRUE" || m_value == "1");
    evaluateValidator(tempb);

    b = (m_value == "true" || m_value == "TRUE" || m_value == "1");
  }
  /// Apply if vector
  void apply(std::vector<double> &v) const override {
    if (m_value.empty() || m_value == "EMPTY") {
      v.clear();
      return;
    }
    if (m_value.size() > 2) {
      // check if the value is in brackets (...)
      if (m_value.front() == '(' && m_value.back() == ')') {
        m_value.erase(0, 1);
        m_value.erase(m_value.size() - 1);
      }
    }
    Kernel::StringTokenizer tokenizer(m_value, ",", Kernel::StringTokenizer::TOK_TRIM);
    size_t newSize = tokenizer.count();

    // if visitor has an associated validator, first populate temp vec and evaluate against validator.
    if (m_validator != nullptr) {
      std::vector<double> tempVec(newSize);

      for (size_t i = 0; i < tempVec.size(); ++i) {
        tempVec[i] = boost::lexical_cast<double>(tokenizer[i]);
      }
      evaluateValidator(tempVec);
    }

    v.resize(newSize);
    for (size_t i = 0; i < v.size(); ++i) {
      v[i] = boost::lexical_cast<double>(tokenizer[i]);
    }
  }

  /// Evaluates the validator associated with this attribute with regards to input value. Returns error as a string.
  template <typename T> void evaluateValidator(T &inputData) const {
    if (m_validator != nullptr) {
      IFunction::ValidatorEvaluator::evaluate(inputData, m_validator);
    }
  }

private:
  mutable std::string m_value; ///< the value as a string
  mutable Kernel::IValidator_sptr m_validator;
};
} // namespace

/** Set value from a string. Throws exception if the string has wrong format
 * @param str :: String representation of the new value
 */
void IFunction::Attribute::fromString(const std::string &str) {
  SetValue tmp(str, m_validator);
  apply(tmp);
}

/** Set validator to enforce limits on attribute value
 * @param validator :: shared ptr to validator object
 */
void IFunction::Attribute::setValidator(const Kernel::IValidator_sptr &validator) const { m_validator = validator; }

/**
 *  Evaluates the validator associated with this attribute.
 */
void IFunction::Attribute::evaluateValidator() const { boost::apply_visitor(AttributeValidatorVisitor(this), m_data); }

/// Value of i-th active parameter. Override this method to make fitted
/// parameters different from the declared
double IFunction::activeParameter(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter " + parameterName(i));
  }
  return getParameter(i);
}

/// Set new value of i-th active parameter. Override this method to make fitted
/// parameters different from the declared
void IFunction::setActiveParameter(size_t i, double value) {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter " + parameterName(i));
  }
  setParameter(i, value);
}

/**
 * Returns the name of an active parameter.
 * @param i :: Index of a parameter. The parameter must be active.
 */
std::string IFunction::nameOfActive(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter " + parameterName(i));
  }
  return parameterName(i);
}

/**
 * Returns the description of an active parameter.
 * @param i :: Index of a parameter. The parameter must be active.
 */
std::string IFunction::descriptionOfActive(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter " + parameterName(i));
  }
  return parameterDescription(i);
}

/** Calculate numerical derivatives.
 * @param domain :: The domain of the function
 * @param jacobian :: A Jacobian matrix. It is expected to have dimensions of
 * domain.size() by nParams().
 */
void IFunction::calNumericalDeriv(const FunctionDomain &domain, Jacobian &jacobian) {
  /*
   * There is a similar more specialized method for 1D functions in IFunction1D
   * but the method takes different parameters and uses slightly different
   * function calls in places making it difficult to share code. Please also
   * consider that method when updating this.
   */

  const size_t nParam = nParams();
  size_t nData = getValuesSize(domain);

  FunctionValues minusStep(nData);
  FunctionValues plusStep(nData);

  applyTies(); // just in case
  function(domain, minusStep);

  if (nData == 0) {
    nData = minusStep.size();
  }

  double step;
  for (size_t iP = 0; iP < nParam; iP++) {
    if (isActive(iP)) {
      const double val = activeParameter(iP);
      step = calculateStepSize(val);

      const double paramPstep = val + step;
      setActiveParameter(iP, paramPstep);
      applyTies();
      function(domain, plusStep);
      setActiveParameter(iP, val);
      applyTies();

      step = paramPstep - val;
      for (size_t i = 0; i < nData; i++) {
        jacobian.set(i, iP, (plusStep.getCalculated(i) - minusStep.getCalculated(i)) / step);
      }
    }
  }
}

/** Calculates the step size to use when calculating the numerical derivative.
 * @param parameterValue :: The value of the active parameter.
 * @returns The step size to use when calculating the numerical derivative.
 */
double IFunction::calculateStepSize(const double parameterValue) const { return m_stepSizeFunction(parameterValue); }

/** Sets the function to use when calculating the step size.
 * @param method :: An enum indicating which method to use when calculating the step size.
 */
void IFunction::setStepSizeMethod(const StepSizeMethod method) {
  switch (method) {
  case StepSizeMethod::DEFAULT:
    m_stepSizeFunction = defaultStepSize;
    return;
  case StepSizeMethod::SQRT_EPSILON:
    m_stepSizeFunction = sqrtEpsilonStepSize;
    return;
  }
  throw std::invalid_argument("An invalid method for calculating the step size was provided.");
}

/** Initialize the function providing it the workspace
 * @param workspace :: The workspace to set
 * @param wi :: The workspace index
 * @param startX :: The lower bin index
 * @param endX :: The upper bin index
 */
void IFunction::setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                                   double endX) {
  UNUSED_ARG(startX);
  UNUSED_ARG(endX);

  if (!workspace)
    return; // unset the workspace

  try {

    // check if parameter are specified in instrument definition file

    const auto &paramMap = workspace->constInstrumentParameters();

    Geometry::IDetector const *detectorPtr = nullptr;
    size_t numDetectors = workspace->getSpectrum(wi).getDetectorIDs().size();
    if (numDetectors > 1) {
      // If several detectors are on this workspace index, just use the ID of
      // the first detector
      // Note JZ oct 2011 - I'm not sure why the code uses the first detector
      // and not the group. Ask Roman.
      auto firstDetectorId = *workspace->getSpectrum(wi).getDetectorIDs().begin();

      const auto &detectorInfo = workspace->detectorInfo();
      const auto detectorIndex = detectorInfo.indexOf(firstDetectorId);
      const auto &detector = detectorInfo.detector(detectorIndex);
      detectorPtr = &detector;
    } else {
      // Get the detector (single) at this workspace index
      const auto &spectrumInfo = workspace->spectrumInfo();
      const auto &detector = spectrumInfo.detector(wi);
      detectorPtr = &detector;
    }

    for (size_t i = 0; i < nParams(); i++) {
      if (!isExplicitlySet(i)) {
        Geometry::Parameter_sptr param = paramMap.getRecursive(detectorPtr, parameterName(i), "fitting");
        if (param != Geometry::Parameter_sptr()) {
          // get FitParameter
          const auto &fitParam = param->value<Geometry::FitParameter>();

          // check first if this parameter is actually specified for this
          // function
          if (name() == fitParam.getFunction()) {
            // update value
            auto *testWithLocation = dynamic_cast<IFunctionWithLocation *>(this);
            if (testWithLocation == nullptr ||
                (!fitParam.getLookUpTable().containData() && fitParam.getFormula().empty())) {
              setParameter(i, fitParam.getValue());
            } else {
              double centreValue = testWithLocation->centre();
              Kernel::Unit_sptr centreUnit; // unit of value used in formula or
                                            // to look up value in lookup table
              if (fitParam.getFormula().empty())
                centreUnit = fitParam.getLookUpTable().getXUnit(); // from table
              else {
                if (!fitParam.getFormulaUnit().empty()) {
                  try {
                    centreUnit = Kernel::UnitFactory::Instance().create(fitParam.getFormulaUnit()); // from formula
                  } catch (...) {
                    g_log.warning() << fitParam.getFormulaUnit() << " Is not an recognised formula unit for parameter "
                                    << fitParam.getName() << "\n";
                  }
                }
              }

              // if unit specified convert centre value to unit required by
              // formula or look-up-table
              if (centreUnit) {
                g_log.debug() << "For FitParameter " << parameterName(i)
                              << " centre of peak before any unit conversion is " << centreValue << '\n';
                centreValue = convertValue(centreValue, centreUnit, workspace, wi);
                g_log.debug() << "For FitParameter " << parameterName(i)
                              << " centre of peak after any unit conversion is " << centreValue << '\n';
              }

              double paramValue = fitParam.getValue(centreValue);

              // this returned param value by a formula or a look-up-table may
              // have
              // a unit of its own. If set convert param value
              // See section 'Using fitting parameters in
              // docs/source/concepts/InstrumentDefinitionFile.rst
              if (fitParam.getFormula().empty()) {
                // so from look up table
                Kernel::Unit_sptr resultUnit = fitParam.getLookUpTable().getYUnit(); // from table
                g_log.debug() << "The FitParameter " << parameterName(i) << " = " << paramValue
                              << " before y-unit conversion\n";
                paramValue /= convertValue(1.0, resultUnit, workspace, wi);
                g_log.debug() << "The FitParameter " << parameterName(i) << " = " << paramValue
                              << " after y-unit conversion\n";
              } else {
                // so from formula

                std::string resultUnitStr = fitParam.getResultUnit();

                if (!resultUnitStr.empty()) {
                  std::vector<std::string> allUnitStr = Kernel::UnitFactory::Instance().getKeys();
                  for (auto &iUnit : allUnitStr) {
                    size_t found = resultUnitStr.find(iUnit);
                    if (found != std::string::npos) {
                      size_t len = iUnit.size();
                      std::stringstream readDouble;
                      Kernel::Unit_sptr unt = Kernel::UnitFactory::Instance().create(iUnit);
                      readDouble << 1.0 / convertValue(1.0, unt, workspace, wi);
                      resultUnitStr.replace(found, len, readDouble.str());
                    }
                  } // end for

                  try {
                    mu::Parser p;
                    p.SetExpr(resultUnitStr);
                    g_log.debug() << "The FitParameter " << parameterName(i) << " = " << paramValue
                                  << " before result-unit conversion (using " << resultUnitStr << ")\n";
                    paramValue *= p.Eval();
                    g_log.debug() << "The FitParameter " << parameterName(i) << " = " << paramValue
                                  << " after result-unit conversion\n";
                  } catch (mu::Parser::exception_type &e) {
                    g_log.error() << "Cannot convert formula unit to workspace unit"
                                  << " Formula unit which cannot be passed is " << resultUnitStr
                                  << ". Muparser error message is: " << e.GetMsg() << '\n';
                  }
                } // end if
              }   // end trying to convert result-unit from formula or y-unit for
              // lookuptable

              setParameter(i, paramValue);
            } // end of update parameter value

            // add tie if specified for this parameter in instrument definition
            // file
            if (!fitParam.getTie().empty()) {
              std::ostringstream str;
              str << getParameter(i);
              tie(parameterName(i), str.str());
            }

            // add constraint if specified for this parameter in instrument
            // definition file
            if (!fitParam.getConstraint().empty()) {
              IConstraint *constraint = ConstraintFactory::Instance().createInitialized(this, fitParam.getConstraint());
              if (!fitParam.getConstraintPenaltyFactor().empty()) {
                try {
                  double penalty = std::stod(fitParam.getConstraintPenaltyFactor());
                  constraint->setPenaltyFactor(penalty);
                } catch (...) {
                  g_log.warning() << "Can't set penalty factor for constraint\n";
                }
              }
              addConstraint(std::unique_ptr<IConstraint>(constraint));
            }
          }
        }
      }
    }
  } catch (...) {
  }
}

/** Convert a value from unit defined in workspace (ws) to outUnit
 *
 *  @param value ::   assumed to be in unit of workspace
 *  @param outUnit ::  unit to convert to
 *  @param ws ::      workspace
 *  @param wsIndex :: workspace index
 *  @return converted value
 */
double IFunction::convertValue(double value, Kernel::Unit_sptr &outUnit,
                               const std::shared_ptr<const MatrixWorkspace> &ws, size_t wsIndex) const {
  // only required if formula or look-up-table different from ws unit
  const auto &wsUnit = ws->getAxis(0)->unit();
  if (outUnit->unitID() == wsUnit->unitID())
    return value;

  // first check if it is possible to do a quick conversion and convert
  // slight duplication to below to avoid instantiating vector unless necessary
  double factor(0.0), power(0.0);
  if (wsUnit->quickConversion(*outUnit, factor, power)) {
    return factor * std::pow(value, power);
  } else {
    std::vector<double> singleValue(1, value);
    convertValue(singleValue, outUnit, ws, wsIndex);
    return singleValue.front();
  }
}

/** Convert values from unit defined in workspace (ws) to outUnit
 *
 *  @param values ::   As input: assumed to be in unit of workspace.
 *                  As output: in unit of outUnit
 *  @param outUnit ::  unit to convert to
 *  @param ws ::      workspace
 *  @param wsIndex :: workspace index
 */
void IFunction::convertValue(std::vector<double> &values, Kernel::Unit_sptr &outUnit,
                             const std::shared_ptr<const MatrixWorkspace> &ws, size_t wsIndex) const {
  // only required if  formula or look-up-table different from ws unit
  const auto &wsUnit = ws->getAxis(0)->unit();
  if (outUnit->unitID() == wsUnit->unitID())
    return;

  // first check if it is possible to do a quick conversion convert
  double factor, power;
  if (wsUnit->quickConversion(*outUnit, factor, power)) {
    auto iend = values.end();
    for (auto itr = values.begin(); itr != iend; ++itr)
      (*itr) = factor * std::pow(*itr, power);
  } else {
    // Get l1, l2 and theta  (see also RemoveBins.calculateDetectorPosition())
    Instrument_const_sptr instrument = ws->getInstrument();
    Geometry::IComponent_const_sptr sample = instrument->getSample();
    if (sample == nullptr) {
      g_log.error() << "No sample defined instrument. Cannot convert units for function\n"
                    << "Ignore conversion.";
      return;
    }
    const auto &spectrumInfo = ws->spectrumInfo();
    double l1 = spectrumInfo.l1();
    // If this is a monitor then l1+l2 = source-detector distance and twoTheta=0
    auto emode = ws->getEMode();

    Kernel::UnitParametersMap pmap{};
    spectrumInfo.getDetectorValues(*wsUnit, *outUnit, emode, false, wsIndex, pmap);
    try {
      std::vector<double> emptyVec;
      wsUnit->toTOF(values, emptyVec, l1, emode, pmap);
      outUnit->fromTOF(values, emptyVec, l1, emode, pmap);
    } catch (std::exception &) {
      throw std::runtime_error("Unable to perform unit conversion to " + outUnit->unitID());
    }
  }
}

/**
 * Returns the number of attributes associated with the function
 */
size_t IFunction::nAttributes() const { return m_attrs.size(); }

/// Check if attribute named exists
bool IFunction::hasAttribute(const std::string &name) const { return m_attrs.find(name) != m_attrs.end(); }

/**
 * Overload for const char* values.
 * @param attName :: Attribute name
 * @param value :: New attribute value to set
 */
void IFunction::setAttributeValue(const std::string &attName, const char *value) {
  std::string str(value);
  setAttributeValue(attName, str);
}

/**
 * Set string attribute by value. Make sure that quoted style doesn't change.
 * @param attName :: Attribute name
 * @param value :: New attribute value to set
 */
void IFunction::setAttributeValue(const std::string &attName, const std::string &value) {
  Attribute att = getAttribute(attName);
  att.setString(value);
  setAttribute(attName, att);
}

/// Returns the pointer to a child function
IFunction_sptr IFunction::getFunction(std::size_t) const {
  throw std::runtime_error("Function " + name() + " doesn't have children.");
}

/// Returns a list of attribute names
std::vector<std::string> IFunction::getAttributeNames() const {
  std::vector<std::string> names;
  names.reserve(nAttributes());
  for (size_t i = 0; i < nAttributes(); ++i) {
    names.emplace_back(attributeName(i));
  }
  return names;
}

/**
 * Return the name of the ith attribute by querying the stored attributes in
 * m_attrs
 * @param index :: Index of the attribute to return
 */
std::string IFunction::attributeName(size_t index) const {
  if (index >= nAttributes()) {
    throw std::out_of_range("Function attribute index out of range.");
  }
  auto itr = std::next(m_attrs.begin(), index);
  return itr->first;
}

/**
 * Return a value of attribute attName
 * @param name :: Returns the named attribute
 */
API::IFunction::Attribute IFunction::getAttribute(const std::string &name) const {
  if (hasAttribute(name)) {
    return m_attrs.at(name);
  } else {
    throw std::invalid_argument("ParamFunctionAttributeHolder::getAttribute - Unknown attribute '" + name + "'");
  }
}

/**
 *  Set a value to a named attribute. Can be overridden in the inheriting class,
 * the default
 *  just stores the value
 *  @param name :: The name of the attribute
 *  @param value :: The value of the attribute
 */
void IFunction::setAttribute(const std::string &name, const API::IFunction::Attribute &value) {
  storeAttributeValue(name, value);
}

/**
 * Declares a single attribute
 * @param name :: The name of the attribute
 * @param defaultValue :: A default value
 */
void IFunction::declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue) {
  checkAttributeName(name);

  m_attrs.emplace(name, defaultValue);
}

/**
 * Declares a single attribute with a validator
 * @param name :: The name of the attribute
 * @param defaultValue :: A default value
 * @param validator :: validator to restrict allows input value of defaultValue param
 */
void IFunction::declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue,
                                 const Kernel::IValidator &validator) {
  const Kernel::IValidator_sptr validatorClone = validator.clone();
  checkAttributeName(name);

  defaultValue.setValidator(validatorClone);
  defaultValue.evaluateValidator();

  m_attrs.emplace(name, defaultValue);
}

/**
 * Checks Attribute of "name" does not exist
 * @param name :: The name of the attribute
 */
void IFunction::checkAttributeName(const std::string &name) {
  if (m_attrs.find(name) != m_attrs.end()) {
    std::ostringstream msg;
    msg << "Attribute (" << name << ") already exists.";
    throw std::invalid_argument(msg.str());
  }
}

/// Initialize the function. Calls declareAttributes & declareParameters
void IFunction::init() {
  declareAttributes();
  declareParameters();
}

/**
 *  Set a value to a named attribute, retaining validator
 *  @param name :: The name of the attribute
 *  @param value :: The value of the attribute
 */
void IFunction::storeAttributeValue(const std::string &name, const API::IFunction::Attribute &value) {
  if (hasAttribute(name)) {
    auto att = m_attrs[name];
    const Kernel::IValidator_sptr validatorClone = att.getValidator();
    value.setValidator(validatorClone);
    value.evaluateValidator();

    m_attrs[name] = value;
  } else {
    throw std::invalid_argument("ParamFunctionAttributeHolder::setAttribute - Unknown attribute '" + name + "'");
  }
}

/**
 *  Store a value to a named attribute if it can be considered "mutable" or
 *  read only, which simply reflects the current state of the function.
 *  @param name :: The name of the attribute
 *  @param value :: The value of the attribute
 */
void IFunction::storeReadOnlyAttribute(const std::string &name, const API::IFunction::Attribute &value) const {
  const_cast<IFunction *>(this)->storeAttributeValue(name, value);
}

/**
 * Set the covariance matrix. Algorithm Fit sets this matrix to the top-level
 * function
 * after fitting. If the function is composite the matrix isn't set to its
 * members.
 * The matrix must be square and its size equal to the number of parameters of
 * this function.
 * @param covar :: A matrix to set.
 */
void IFunction::setCovarianceMatrix(const std::shared_ptr<Kernel::Matrix<double>> &covar) {
  // the matrix shouldn't be empty
  if (!covar) {
    throw std::invalid_argument("IFunction: Cannot set an empty covariance matrix");
  }
  // the matrix should relate to this function
  if (covar->numRows() != nParams() || covar->numCols() != nParams()) {
    throw std::invalid_argument("IFunction: Covariance matrix has a wrong size");
  }
  m_covar = covar;
}

/// Get number of values for a given domain.
/// @param domain :: A domain.
size_t IFunction::getValuesSize(const FunctionDomain &domain) const { return domain.size(); }

/// Fix a parameter
/// @param name :: A name of a parameter to fix
/// @param isDefault :: If true fix it by default
void IFunction::fixParameter(const std::string &name, bool isDefault) {
  auto i = parameterIndex(name);
  fix(i, isDefault);
}

/// Free a parameter
/// @param name :: A name of a parameter to free
void IFunction::unfixParameter(const std::string &name) {
  auto i = parameterIndex(name);
  unfix(i);
}

/// Fix all parameters
/// @param isDefault :: If true fix them by default
void IFunction::fixAll(bool isDefault) {
  for (size_t i = 0; i < nParams(); ++i) {
    if (isActive(i)) {
      fix(i, isDefault);
    }
  }
}

/// Free all parameters
void IFunction::unfixAll() {
  for (size_t i = 0; i < nParams(); ++i) {
    if (isFixed(i)) {
      unfix(i);
    }
  }
}

/// Free all parameters fixed by default
void IFunction::unfixAllDefault() {
  for (size_t i = 0; i < nParams(); ++i) {
    if (getParameterStatus(i) == FixedByDefault) {
      unfix(i);
    }
  }
}

/// Fix all active parameters. This method doesn't change
/// status of a fixed parameter, eg if one was fixed by default
/// prior to calling this method it will remain default regardless
/// the value of isDefault argument.
/// @param isDefault :: If true fix them by default.
void IFunction::fixAllActive(bool isDefault) {
  for (size_t i = 0; i < nParams(); ++i) {
    if (getParameterStatus(i) == Active) {
      fix(i, isDefault);
    }
  }
}

/// Get number of domains required by this function.
/// If it returns a number greater than 1 then the domain
/// passed to function(domain, values) method must have a
/// CompositeDomain type with the same number of parts.
size_t IFunction::getNumberDomains() const { return 1; }

/// Split this function (if needed) into a list of independent functions.
/// The number of functions must be the number of domains this function is
/// working on (== getNumberDomains()). The result of evaluation of the
/// created functions on their domains must be the same as if this function
/// was evaluated on the composition of those domains.
std::vector<IFunction_sptr> IFunction::createEquivalentFunctions() const {
  return std::vector<IFunction_sptr>(1, FunctionFactory::Instance().createInitialized(asString()));
}

/// Put all ties in order in which they will be applied correctly.
/// @param checkOnly :: If true then do not clear or write to m_orderedTies, only check for circular and self ties.
/// @throws std::runtime_error :: On finding a circular or self tie
void IFunction::sortTies(const bool checkOnly) {
  if (!checkOnly) {
    m_orderedTies.clear();
  }

  std::list<TieNode> orderedTieNodes;
  for (size_t i = 0; i < nParams(); ++i) {
    auto const tie = getTie(i);
    if (!tie || ignoreTie(*tie)) {
      continue;
    }

    TieNode newNode;
    newNode.left = getParameterIndex(*tie);
    auto const rhsParameters = tie->getRHSParameters();
    newNode.right.reserve(rhsParameters.size());
    for (auto &&p : rhsParameters) {
      newNode.right.emplace_back(this->getParameterIndex(p));
    }
    if (newNode < newNode) {
      throw std::runtime_error("Parameter is tied to itself: " + tie->asString(this));
    }
    bool before(false), after(false);
    size_t indexBefore(0), indexAfter(0);
    for (auto &&node : orderedTieNodes) {
      if (newNode < node) {
        before = true;
        indexBefore = node.left;
      }
      if (node < newNode) {
        after = true;
        indexAfter = node.left;
      }
    }
    if (before) {
      if (after) {
        std::string message = "Circular dependency in ties:\n" + tie->asString(this) + '\n';
        message += getTie(indexBefore)->asString(this);
        if (indexAfter != indexBefore) {
          message += '\n' + getTie(indexAfter)->asString(this);
        }
        throw std::runtime_error(message);
      }
      orderedTieNodes.push_front(newNode);
    } else {
      orderedTieNodes.emplace_back(newNode);
    }
  }
  if (!checkOnly) {
    for (auto &&node : orderedTieNodes) {
      auto const tie = getTie(node.left);
      m_orderedTies.emplace_back(tie);
    }
  }
}

} // namespace Mantid::API

///\cond TEMPLATE
namespace Mantid::Kernel {

template <>
MANTID_API_DLL std::shared_ptr<Mantid::API::IFunction>
IPropertyManager::getValue<std::shared_ptr<Mantid::API::IFunction>>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<std::shared_ptr<Mantid::API::IFunction>> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<IFunction>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL std::shared_ptr<const Mantid::API::IFunction>
IPropertyManager::getValue<std::shared_ptr<const Mantid::API::IFunction>>(const std::string &name) const {
  const auto *prop =
      dynamic_cast<PropertyWithValue<std::shared_ptr<Mantid::API::IFunction>> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<IFunction>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel
///\endcond TEMPLATE
