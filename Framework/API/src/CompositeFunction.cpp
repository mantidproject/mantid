// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>

namespace Mantid::API {

namespace {
/// static logger
Kernel::Logger g_log("CompositeFunction");
constexpr const char *ATTNUMDERIV = "NumDeriv";
constexpr int NUMDEFAULTATTRIBUTES = 1;
/**
 * Helper function called when we replace a function within the composite
 * function For example, consider a composite function with 5 attributes, 3
 * (oldSize) belonging to the first function and two to the second i.e. the
 * variableFunctionIndexList is [0, 0, 0, 1, 1]. If we replace the second
 * (functionIndex) function with a new function that has 3 (newSize) attributes
 * we need the variableFunctionIndexList to become [0, 0, 0, 1, 1, 1]. This
 * function performs this operation.
 */
template <typename T>
void replaceVariableIndexRange(std::vector<T> &variableFunctionIndexList, const size_t oldSize, const size_t newSize,
                               const T functionIndex) {
  auto itFun = std::find(variableFunctionIndexList.begin(), variableFunctionIndexList.end(), functionIndex);
  if (itFun != variableFunctionIndexList.end()) {
    if (oldSize > newSize) {
      variableFunctionIndexList.erase(itFun, itFun + oldSize - newSize);
    } else if (oldSize < newSize) {
      variableFunctionIndexList.insert(itFun, newSize - oldSize, functionIndex);
    }
  } else if (newSize > 0) {
    using std::placeholders::_1;
    itFun = std::find_if(variableFunctionIndexList.begin(), variableFunctionIndexList.end(),
                         std::bind(std::greater<size_t>(), _1, functionIndex));
    variableFunctionIndexList.insert(itFun, newSize, functionIndex);
  }
}

} // namespace

using std::size_t;

DECLARE_FUNCTION(CompositeFunction)

/// Default constructor
CompositeFunction::CompositeFunction() : IFunction(), m_nParams(0), m_nAttributes(0), m_iConstraintFunction(false) {
  createDefaultGlobalAttributes();
}

void CompositeFunction::createDefaultGlobalAttributes() {
  m_globalAttributeNames.clear();
  declareAttribute(ATTNUMDERIV, Attribute(false));
  m_nAttributes = NUMDEFAULTATTRIBUTES;
}

/// Function initialization. Declare function parameters in this method.
void CompositeFunction::init() {}

/**
 * Writes itself into a string. Functions derived from CompositeFunction may
 * need to override this method with something like this:
 *   std::string NewFunction::writeToString()const
 *   {
 *      ostr << "composite=" << this->name() << ';';
 *      // write NewFunction's own attributes and parameters
 *      ostr << CompositeFunction::asString();
 *      // write NewFunction's own ties and constraints
 *      // ostr << ";constraints=(" << ... <<")";
 *   }
 * @param parentLocalAttributesStr :: A preformatted string with parent's local
 * attributes.
 *    Can be passed in by a CompositeFunction (eg MultiDomainFunction).
 * @return the string representation of the composite function
 */
std::string CompositeFunction::writeToString(const std::string &parentLocalAttributesStr) const {
  std::ostringstream ostr;

  // if empty just return function name
  if (nFunctions() == 0) {
    return "name=" + name();
  }

  if (name() != "CompositeFunction" || nGlobalAttributes() > NUMDEFAULTATTRIBUTES ||
      getAttribute(ATTNUMDERIV).asBool() || !parentLocalAttributesStr.empty()) {
    ostr << "composite=" << name();
    std::vector<std::string> attr = m_globalAttributeNames;
    for (const auto &attName : attr) {
      std::string attValue = this->getAttribute(attName).value();
      if (!attValue.empty()) {
        ostr << ',' << attName << '=' << attValue;
      }
    }
    ostr << parentLocalAttributesStr << ';';
  }
  const auto localAttr = this->getLocalAttributeNames();
  for (size_t i = 0; i < nFunctions(); i++) {
    IFunction_sptr fun = getFunction(i);
    bool isComp = std::dynamic_pointer_cast<CompositeFunction>(fun) != nullptr;
    if (isComp)
      ostr << '(';
    std::ostringstream localAttributesStr;
    for (const auto &localAttName : localAttr) {
      const std::string localAttValue = this->getLocalAttribute(i, localAttName).value();
      if (!localAttValue.empty()) {
        // local attribute names are prefixed by dollar sign
        localAttributesStr << ',' << '$' << localAttName << '=' << localAttValue;
      }
    }
    ostr << fun->writeToString(localAttributesStr.str());
    if (isComp)
      ostr << ')';
    if (i < nFunctions() - 1) {
      ostr << ';';
    }
  }

  // collect non-default constraints
  std::string constraints = writeConstraints();
  // print constraints
  if (!constraints.empty()) {
    ostr << ";constraints=(" << constraints << ")";
  }

  // collect the non-default ties
  std::string ties = writeTies();
  // print the ties
  if (!ties.empty()) {
    ostr << ";ties=(" << ties << ")";
  }

  return ostr.str();
}

/**
 * @param ws A pointer to the workspace being fitted
 */
void CompositeFunction::setWorkspace(std::shared_ptr<const Workspace> ws) {
  // Pass it on to each member
  auto iend = m_functions.end();
  for (auto it = m_functions.begin(); it != iend; ++it) {
    (*it)->setWorkspace(ws);
  }
}

/**
 * @param workspace :: A workspace to fit to.
 * @param wi :: An index of a spectrum to fit to.
 * @param startX :: A start of the fitting region.
 * @param endX :: An end of the fitting region.
 */
void CompositeFunction::setMatrixWorkspace(std::shared_ptr<const MatrixWorkspace> workspace, size_t wi, double startX,
                                           double endX) {
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    m_functions[iFun]->setMatrixWorkspace(workspace, wi, startX, endX);
  }
}

/** Sets the function to use when calculating the step size.
 * @param stepSizeMethod :: An enum indicating which method to use when calculating the step size.
 */
void CompositeFunction::setStepSizeMethod(const StepSizeMethod stepSizeMethod) {
  std::for_each(m_functions.begin(), m_functions.end(),
                [&stepSizeMethod](const auto &function) { function->setStepSizeMethod(stepSizeMethod); });
}

/** Function you want to fit to.
 *  @param domain :: An instance of FunctionDomain with the function arguments.
 *  @param values :: A FunctionValues instance for storing the calculated
 * values.
 */
void CompositeFunction::function(const FunctionDomain &domain, FunctionValues &values) const {
  FunctionValues tmp(domain);
  values.zeroCalculated();
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    m_functions[iFun]->function(domain, tmp);
    values += tmp;
  }
}

/**
 * Derivatives of function with respect to active parameters
 * @param domain :: Function domain to get the arguments from.
 * @param jacobian :: A Jacobian to store the derivatives.
 */
void CompositeFunction::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) {
  if (getAttribute(ATTNUMDERIV).asBool()) {
    calNumericalDeriv(domain, jacobian);
  } else {
    for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
      PartialJacobian J(&jacobian, paramOffset(iFun));
      getFunction(iFun)->functionDeriv(domain, J);
    }
  }
}

/** Sets a new value to the i-th parameter.
 *  @param i :: The parameter index
 *  @param value :: The new value
 *  @param explicitlySet :: A boolean falgging the parameter as explicitly set
 * (by user)
 */
void CompositeFunction::setParameter(size_t i, const double &value, bool explicitlySet) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setParameter(i - m_paramOffsets[iFun], value, explicitlySet);
}

/** Sets a new description to the i-th parameter.
 *  @param i :: The parameter index
 *  @param description :: The new description
 */
void CompositeFunction::setParameterDescription(size_t i, const std::string &description) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setParameterDescription(i - m_paramOffsets[iFun], description);
}

/** Get the i-th parameter.
 *  @param i :: The parameter index
 *  @return value of the requested parameter
 */
double CompositeFunction::getParameter(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->getParameter(i - m_paramOffsets[iFun]);
}

/** Get the j-th parameter from the i-th function.
 *  @param i :: The function index
 *  @param j :: The i-th function's j-th parameter
 *  @return value of the requested parameter
 */
double CompositeFunction::getParameter(size_t i, size_t j) const { return m_functions[i]->getParameter(j); }

/**
 * Check if function has a parameter with a particular name.
 * @param name :: A name of a parameter.
 * @return True if the parameter exists.
 */
bool CompositeFunction::hasParameter(const std::string &name) const {
  try {
    const auto [parName, index] = parseName(name);
    return index < m_functions.size() ? m_functions[index]->hasParameter(parName) : false;
  } catch (std::invalid_argument &) {
    return false;
  }
}

/**
 * Check if function has a attribute with a particular name.
 * @param name :: A name of a attribute.
 * @return True if the parameter exists.
 */
bool CompositeFunction::hasAttribute(const std::string &name) const {
  try {
    if (std::find(m_globalAttributeNames.begin(), m_globalAttributeNames.end(), name) != m_globalAttributeNames.end()) {
      return true;
    }
    const auto [attrName, index] = parseName(name);
    return index < m_functions.size() ? m_functions[index]->hasAttribute(attrName) : false;
  } catch (std::invalid_argument &) {
    return false;
  }
}

/**
 * Sets a new value to a parameter by name.
 * @param name :: The name of the parameter.
 * @param value :: The new value
 * @param explicitlySet :: A boolean flagging the parameter as explicitly set
 * (by user)
 */
void CompositeFunction::setParameter(const std::string &name, const double &value, bool explicitlySet) {
  const auto [parName, index] = parseName(name);
  getFunction(index)->setParameter(parName, value, explicitlySet);
}

/**
 * Sets a new description to a parameter by name.
 * @param name :: The name of the parameter.
 * @param description :: The new description
 */
void CompositeFunction::setParameterDescription(const std::string &name, const std::string &description) {
  const auto [parName, index] = parseName(name);
  getFunction(index)->setParameterDescription(parName, description);
}

/**
 * Parameters by name.
 * @param name :: The name of the parameter.
 * @return value of the requested named parameter
 */
double CompositeFunction::getParameter(const std::string &name) const {
  const auto [parName, index] = parseName(name);
  return getFunction(index)->getParameter(parName);
}

/**
 * Return a value of attribute attName
 * @param name :: Returns the named attribute
 */
API::IFunction::Attribute CompositeFunction::getAttribute(const std::string &name) const {
  try {
    if (std::find(m_globalAttributeNames.begin(), m_globalAttributeNames.end(), name) != m_globalAttributeNames.end()) {
      return IFunction::getAttribute(name);
    }
    const auto [attrName, index] = parseName(name);
    return m_functions[index]->getAttribute(attrName);
  } catch (std::invalid_argument &) {
    throw std::invalid_argument("ParamFunctionAttributeHolder::getAttribute - Unknown attribute '" + name + "'");
  }
}

/**
 *  Set a value of a named attribute.
 *  @param name :: The name of the attribute
 *  @param value :: The value of the attribute
 */
void CompositeFunction::setAttribute(const std::string &name, const API::IFunction::Attribute &value) {
  if (std::find(m_globalAttributeNames.begin(), m_globalAttributeNames.end(), name) != m_globalAttributeNames.end()) {
    return IFunction::setAttribute(name, value);
  }
  const auto [attrName, index] = parseName(name);
  return m_functions[index]->setAttribute(attrName, value);
}

/// Total number of parameters
size_t CompositeFunction::nParams() const { return m_nParams; }

// Total number of attributes
size_t CompositeFunction::nAttributes() const {
  return std::accumulate(
      m_functions.cbegin(), m_functions.cend(), nGlobalAttributes(),
      [](const size_t accumulator, const auto &function) -> size_t { return accumulator + function->nAttributes(); });
}
/**
 *
 * @param name :: The name of a parameter
 * @return index of the requested named parameter
 */
size_t CompositeFunction::parameterIndex(const std::string &name) const {
  const auto [parName, index] = parseName(name);
  return getFunction(index)->parameterIndex(parName) + m_paramOffsets[index];
}

/// Returns the name of parameter
/// @param i :: The index
/// @return The name of the parameter
std::string CompositeFunction::parameterName(size_t i) const {
  const size_t iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.' << m_functions[iFun]->parameterName(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/// Returns the name of i-th function's j-th parameter
/// @param i :: The function index
/// @param j :: The local index of the parameter relative to the function index
/// @return The name of the parameter
std::string CompositeFunction::parameterName(size_t i, size_t j) const {
  std::ostringstream ostr;
  ostr << 'f' << i << '.' << m_functions[i]->parameterName(j);
  return ostr.str();
}

/// Returns the name of the ith attribute
/// @param index :: The index of the attribute
/// @return The name of the attribute
std::string CompositeFunction::attributeName(size_t index) const {
  if (index < nGlobalAttributes())
    return IFunction::attributeName(index);

  // Offset the index by the number of global attributes
  const size_t offsetIndex = index - nGlobalAttributes();
  size_t funcIndex = attributeFunctionIndex(offsetIndex);
  std::ostringstream ostr;
  ostr << 'f' << funcIndex << '.' << m_functions[funcIndex]->attributeName(getAttributeOffset(offsetIndex));
  return ostr.str();
}

/// Returns the description of parameter
/// @param i :: The index
/// @return The description of the parameter
std::string CompositeFunction::parameterDescription(size_t i) const {
  const size_t iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << m_functions[iFun]->parameterDescription(i - m_paramOffsets[iFun]);
  return ostr.str();
}
/**
 * Get the fitting error for a parameter
 * @param i :: The index of a parameter
 * @return :: the error
 */
double CompositeFunction::getError(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->getError(i - m_paramOffsets[iFun]);
}
/**
 * Get the fitting error for the i-th function's j-th parameter
 * @param i :: The index of the i-th function
 * @param j :: The index of the i-th function's j-th parameter
 * @return :: the error
 */
double CompositeFunction::getError(size_t i, size_t j) const { return m_functions[i]->getError(j); }

/**
 * Get the fitting error for a parameter by name.
 * @param name :: The name of the parameter.
 * @return value of the requested named parameter
 */
double CompositeFunction::getError(const std::string &name) const {
  const auto [parName, index] = parseName(name);
  return getFunction(index)->getError(parName);
}

/**
 * Set the fitting error for a parameter
 * @param i :: The index of a parameter
 * @param err :: The error value to set
 */
void CompositeFunction::setError(size_t i, double err) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setError(i - m_paramOffsets[iFun], err);
}

/**
 * Sets the fitting error to a parameter by name.
 * @param name :: The name of the parameter.
 * @param err :: The error value to set
 */
void CompositeFunction::setError(const std::string &name, double err) {
  auto [parName, index] = parseName(name);
  getFunction(index)->setError(parName, err);
}

/// Value of i-th active parameter. Override this method to make fitted
/// parameters different from the declared
double CompositeFunction::activeParameter(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->activeParameter(i - m_paramOffsets[iFun]);
}

/// Set new value of i-th active parameter. Override this method to make
/// fitted parameters different from the declared
void CompositeFunction::setActiveParameter(size_t i, double value) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setActiveParameter(i - m_paramOffsets[iFun], value);
}

/// Returns the name of active parameter i
std::string CompositeFunction::nameOfActive(size_t i) const {
  size_t iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.' << m_functions[iFun]->nameOfActive(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/// Returns the description of active parameter i
std::string CompositeFunction::descriptionOfActive(size_t i) const {
  size_t iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << m_functions[iFun]->descriptionOfActive(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/// Change status of parameter
void CompositeFunction::setParameterStatus(size_t i, IFunction::ParameterStatus status) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setParameterStatus(i - m_paramOffsets[iFun], status);
}

/// Get status of parameter
IFunction::ParameterStatus CompositeFunction::getParameterStatus(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->getParameterStatus(i - m_paramOffsets[iFun]);
}

/** Makes sure that the function is consistent.
 */
void CompositeFunction::checkFunction() {
  m_nParams = 0;
  m_nAttributes = nGlobalAttributes();
  m_paramOffsets.clear();
  m_IFunction.clear();
  m_attributeIndex.clear();

  std::vector<IFunction_sptr> functions(m_functions.begin(), m_functions.end());
  m_functions.clear();

  for (auto &f : functions) {
    CompositeFunction_sptr cf = std::dynamic_pointer_cast<CompositeFunction>(f);
    if (cf)
      cf->checkFunction();
    addFunction(f);
  }
}

/**
 * Remove all member functions
 */
void CompositeFunction::clear() {
  m_nParams = 0;
  m_nAttributes = nGlobalAttributes();
  m_paramOffsets.clear();
  m_IFunction.clear();
  m_functions.clear();
  m_attributeIndex.clear();
}

/** Add a function
 * @param f :: A pointer to the added function
 * @return The function index
 */
size_t CompositeFunction::addFunction(IFunction_sptr f) {
  m_IFunction.insert(m_IFunction.end(), f->nParams(), m_functions.size());
  m_attributeIndex.insert(m_attributeIndex.end(), f->nAttributes(), m_functions.size());
  m_functions.emplace_back(f);
  if (m_paramOffsets.empty()) {
    m_paramOffsets.emplace_back(0);
    m_nParams = f->nParams();
    m_nAttributes = f->nAttributes() + nGlobalAttributes();
  } else {
    m_paramOffsets.emplace_back(m_nParams);
    m_nParams += f->nParams();
    m_nAttributes += f->nAttributes();
  }
  return m_functions.size() - 1;
}

/** Remove a function
 * @param i :: The index of the function to remove
 */
void CompositeFunction::removeFunction(size_t i) {
  if (i >= nFunctions()) {
    throw std::out_of_range("Function index (" + std::to_string(i) + ") out of range (" + std::to_string(nFunctions()) +
                            ").");
  }

  const IFunction_sptr fun = getFunction(i);
  // Remove ties which are no longer valid
  for (size_t j = 0; j < nParams();) {
    ParameterTie *tie = getTie(j);
    if (tie && tie->findParametersOf(fun.get())) {
      removeTie(j);
    } else {
      j++;
    }
  }

  // Shift down the function indices for parameters and attributes
  auto shiftDown = [&](auto &functionIndex) {
    if (functionIndex == i) {
      return true;
    } else if (functionIndex > i) {
      functionIndex -= 1;
      return false;
    } else {
      return false;
    }
  };

  m_IFunction.erase(std::remove_if(m_IFunction.begin(), m_IFunction.end(), shiftDown), m_IFunction.end());
  m_attributeIndex.erase(std::remove_if(m_attributeIndex.begin(), m_attributeIndex.end(), shiftDown),
                         m_attributeIndex.end());

  // Reduction in parameters and attributes
  m_nParams -= fun->nParams();
  m_nAttributes -= fun->nAttributes();

  // Shift the parameter offsets down by the total number of i-th function's
  // params
  for (size_t j = i + 1; j < nFunctions(); j++) {
    m_paramOffsets[j] -= fun->nParams();
  }
  m_paramOffsets.erase(m_paramOffsets.begin() + i);

  m_functions.erase(m_functions.begin() + i);
}

/** Replace a function with a new one. The old function is deleted.
 *  The new function must have already its workspace set.
 * @param f_old :: The pointer to the function to replace. If it's not
 *  a member of this composite function nothing happens
 * @param f_new :: A pointer to the new function
 */
void CompositeFunction::replaceFunctionPtr(const IFunction_sptr &f_old, const IFunction_sptr &f_new) {
  std::vector<IFunction_sptr>::const_iterator it = std::find(m_functions.begin(), m_functions.end(), f_old);
  if (it == m_functions.end())
    return;
  std::vector<IFunction_sptr>::difference_type iFun = it - m_functions.begin();
  replaceFunction(iFun, f_new);
}

/** Replace a function with a new one. The old function is deleted.
 * @param functionIndex :: The index of the function to replace
 * @param f :: A pointer to the new function
 */
void CompositeFunction::replaceFunction(size_t functionIndex, const IFunction_sptr &f) {
  if (functionIndex >= nFunctions()) {
    throw std::out_of_range("Function index (" + std::to_string(functionIndex) + ") out of range (" +
                            std::to_string(nFunctions()) + ").");
  }

  const IFunction_sptr fun = getFunction(functionIndex);
  const size_t np_old = fun->nParams();
  const size_t np_new = f->nParams();
  const size_t at_old = fun->nAttributes();
  const size_t at_new = f->nAttributes();

  // Modify function parameter and attribute indices:
  replaceVariableIndexRange(m_IFunction, np_old, np_new, functionIndex);
  replaceVariableIndexRange(m_attributeIndex, at_old, at_new, functionIndex);

  // Decrement attribute and parameter counts
  const size_t dnp = np_new - np_old;
  const size_t dna = at_new - at_old;
  m_nParams += dnp;
  m_nAttributes += dna;

  // Shift the parameter offsets down by the total number of i-th function's
  // params
  for (size_t j = functionIndex + 1; j < nFunctions(); j++) {
    m_paramOffsets[j] += dnp;
  }
  m_functions[functionIndex] = f;
}

/**
 * @param functionName :: The function name to search for.
 * @returns true if the composite function has at least one of a function with a
 * matching name.
 */
bool CompositeFunction::hasFunction(const std::string &functionName) const {
  return std::any_of(m_functions.cbegin(), m_functions.cend(), [&functionName](const IFunction_const_sptr &function) {
    return function->name() == functionName;
  });
}

/**
 * @param i :: The index of the function
 * @return function at the requested index
 */
IFunction_sptr CompositeFunction::getFunction(std::size_t i) const {
  if (i >= nFunctions()) {
    throw std::out_of_range("Function index (" + std::to_string(i) + ") out of range (" + std::to_string(nFunctions()) +
                            ").");
  }
  return m_functions[i];
}

/**
 * Gets the index of the first function with a matching function string.
 * @param functionName :: The name of the function to search for.
 * @returns function index of the first function with a matching function
 * string.
 */
std::size_t CompositeFunction::functionIndex(const std::string &functionName) const {
  const auto iter =
      std::find_if(m_functions.cbegin(), m_functions.cend(),
                   [&functionName](const IFunction_const_sptr &function) { return function->name() == functionName; });

  if (iter != m_functions.cend())
    return std::distance(m_functions.cbegin(), iter);

  throw std::invalid_argument("A function with name '" + functionName + "' does not exist in this composite function.");
}

/**
 * Get the index of the function to which parameter i belongs
 * @param i :: The parameter index
 * @return function index of the requested parameter
 */
size_t CompositeFunction::functionIndex(std::size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("Function parameter index (" + std::to_string(i) + ") out of range (" +
                            std::to_string(nParams()) + ").");
  }
  return m_IFunction[i];
}
/**
 * Get the index of the function to which parameter i belongs
 * @param i :: The parameter index
 * @return function index of the requested parameter
 */
size_t CompositeFunction::attributeFunctionIndex(std::size_t i) const {
  if (i >= nAttributes()) {
    throw std::out_of_range("Function attribute index (" + std::to_string(i) + ") out of range (" +
                            std::to_string(nAttributes()) + ").");
  }
  return m_attributeIndex[i];
}

/**
 * @param varName :: The variable name which may contain function index (
 * [f<index.>]name )
 * @return pair containing the unprefixed variable name and functionIndex
 */
std::pair<std::string, size_t> CompositeFunction::parseName(const std::string &varName) {
  const size_t i = varName.find('.');
  if (i == std::string::npos) {
    throw std::invalid_argument("Variable " + varName + " not found.");
  } else {
    if (varName[0] != 'f')
      throw std::invalid_argument("External function variable name must start with 'f'");

    const std::string sindex = varName.substr(1, i - 1);
    const size_t index = boost::lexical_cast<size_t>(sindex);

    if (i == varName.size() - 1)
      throw std::invalid_argument("Name cannot be empty");

    return std::make_pair(varName.substr(i + 1), index);
  }
}

/** Returns the index of parameter i as it declared in its function
 * @param i :: The parameter index
 * @param recursive :: If true call parameterLocalName recursively until
 *    a non-composite function is reached.
 * @return The local index of the parameter
 */
size_t CompositeFunction::parameterLocalIndex(size_t i, bool recursive) const {
  size_t iFun = functionIndex(i);
  auto localIndex = i - m_paramOffsets[iFun];
  if (recursive) {
    auto cf = dynamic_cast<const CompositeFunction *>(m_functions[iFun].get());
    if (cf) {
      return cf->parameterLocalIndex(localIndex, recursive);
    }
  }
  return localIndex;
}

/** Returns the name of parameter i as it declared in its function
 * @param i :: The parameter index
 * @param recursive :: If true call parameterLocalName recursively until
 *    a non-composite function is reached.
 * @return The pure parameter name (without the function identifier f#.)
 */
std::string CompositeFunction::parameterLocalName(size_t i, bool recursive) const {
  size_t iFun = functionIndex(i);
  auto localIndex = i - m_paramOffsets[iFun];
  auto localFunction = m_functions[iFun].get();
  if (recursive) {
    auto cf = dynamic_cast<const CompositeFunction *>(localFunction);
    if (cf) {
      return cf->parameterLocalName(localIndex, recursive);
    }
  }
  return localFunction->parameterName(localIndex);
}

/**
 * Apply the ties.
 */
void CompositeFunction::applyTies() {
  if (hasOrderedTies()) {
    applyOrderedTies();
  } else {
    for (size_t i = 0; i < nFunctions(); i++) {
      getFunction(i)->applyTies();
    }
    IFunction::applyTies();
  }
}

/**
 * Clear the ties.
 */
void CompositeFunction::clearTies() {
  IFunction::clearTies();
  for (size_t i = 0; i < nFunctions(); i++) {
    getFunction(i)->clearTies();
  }
}

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i :: The index of the tied parameter.
 * @return True if successful
 */
bool CompositeFunction::removeTie(size_t i) {
  bool foundAndRemovedTie = false;
  // Handle the case when IFunction::removeTie throws a runtime error because it is trying to unfix a tied parameter
  try {
    foundAndRemovedTie = IFunction::removeTie(i);
  } catch (std::runtime_error &) {
    foundAndRemovedTie = false;
  }
  if (!foundAndRemovedTie) {
    size_t iFun = functionIndex(i);
    bool res = m_functions[iFun]->removeTie(i - m_paramOffsets[iFun]);
    return res;
  }
  return foundAndRemovedTie;
}

/** Get the tie of i-th parameter
 * @param i :: The parameter index
 * @return A pointer to the tie.
 */
ParameterTie *CompositeFunction::getTie(size_t i) const {
  auto tie = IFunction::getTie(i);
  if (tie == nullptr) {
    size_t iFun = functionIndex(i);
    tie = m_functions[iFun]->getTie(i - m_paramOffsets[iFun]);
  }
  return tie;
}

/**
 * Declare a new parameter. To used in the implementation's constructor.
 * @param name :: The parameter name.
 * @param initValue :: The initial value for the parameter
 * @param description :: Parameter documentation
 */
void CompositeFunction::declareParameter(const std::string &name, double initValue, const std::string &description) {
  (void)name;        // Avoid compiler warning
  (void)initValue;   // Avoid compiler warning
  (void)description; // Avoid compiler warning
  throw Kernel::Exception::NotImplementedError("CompositeFunction cannot not have its own parameters.");
}
/**
 * Declares a single (global) attribute on the composite function
 * @param name :: The name of the attribute
 * @param defaultValue :: A default value
 */
void CompositeFunction::declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue) {
  IFunction::declareAttribute(name, defaultValue);
  m_globalAttributeNames.emplace_back(name);
}

/**
 * Declares a single (global) attribute on the composite function, with a validator
 * @param name :: The name of the attribute
 * @param defaultValue :: A default value
 * @param validator :: validator to restrict attribute values
 */
void CompositeFunction::declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue,
                                         const Kernel::IValidator &validator) {
  IFunction::declareAttribute(name, defaultValue, validator);
  m_globalAttributeNames.emplace_back(name);
}

/**
Registers the usage of the function with the UsageService
 */
void CompositeFunction::registerFunctionUsage(bool internal) {
  for (size_t i = 0; i < nFunctions(); i++) {
    getFunction(i)->registerFunctionUsage(internal);
  }
}

/**
 * Prepare the function for a fit.
 */
void CompositeFunction::setUpForFit() {
  IFunction::setUpForFit();
  // set up the member functions
  for (size_t i = 0; i < nFunctions(); i++) {
    getFunction(i)->setUpForFit();
  }

  // Instead of automatically switching to numeric derivatives
  // log a warning about a danger of not using it
  if (!getAttribute("NumDeriv").asBool()) {
    for (size_t i = 0; i < nParams(); ++i) {
      ParameterTie *tie = getTie(i);
      if (tie && !tie->isConstant()) {
        g_log.warning() << "Numeric derivatives should be used when "
                           "non-constant ties defined.\n";
        break;
      }
    }
  }
}

/// Get constraint
/// @param i :: the index
/// @return A pointer to the constraint
IConstraint *CompositeFunction::getConstraint(size_t i) const {
  auto constraint = IFunction::getConstraint(i);
  if (constraint == nullptr) {
    size_t iFun = functionIndex(i);
    constraint = m_functions[iFun]->getConstraint(i - m_paramOffsets[iFun]);
  }
  return constraint;
}

/** Remove a constraint
 * @param parName :: The name of a parameter which constraint to remove.
 */
void CompositeFunction::removeConstraint(const std::string &parName) {
  auto i = parameterIndex(parName);
  auto constraint = IFunction::getConstraint(i);
  if (constraint != nullptr) {
    IFunction::removeConstraint(parName);
  } else {
    size_t iPar = parameterIndex(parName);
    size_t iFun = functionIndex(iPar);
    getFunction(iFun)->removeConstraint(parameterLocalName(iPar));
  }
}

/** Checks if a constraint has been explicitly set
 *  @param i :: The parameter index
 *  @return true if the function is explicitly set
 */
bool CompositeFunction::isExplicitlySet(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->isExplicitlySet(i - m_paramOffsets[iFun]);
}

/**
 * Returns the index of parameter if the ref points to one of the member
 * function
 * @param ref :: A reference to a parameter
 * @return Parameter index or number of nParams() if parameter not found
 */
size_t CompositeFunction::getParameterIndex(const ParameterReference &ref) const {
  if (ref.getLocalFunction() == this && ref.getLocalIndex() < nParams()) {
    return ref.getLocalIndex();
  }
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    IFunction_sptr fun = getFunction(iFun);
    size_t iLocalIndex = fun->getParameterIndex(ref);
    if (iLocalIndex < fun->nParams()) {
      return m_paramOffsets[iFun] + iLocalIndex;
    }
  }
  return nParams();
}

/**
 * Returns the shared pointer to the function containing a parameter
 * @param ref :: The reference
 * @return A function containing parameter pointed to by ref
 */
IFunction_sptr CompositeFunction::getContainingFunction(const ParameterReference &ref) const {
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    IFunction_sptr fun = getFunction(iFun);
    if (fun->getParameterIndex(ref) < fun->nParams()) {
      return fun;
    }
  }
  return IFunction_sptr();
}

/// Get number of domains required by this function
size_t CompositeFunction::getNumberDomains() const {
  auto n = nFunctions();
  if (n == 0) {
    return 1;
  }
  size_t nd = getFunction(0)->getNumberDomains();
  for (size_t iFun = 1; iFun < n; ++iFun) {
    if (getFunction(iFun)->getNumberDomains() != nd) {
      throw std::runtime_error("CompositeFunction has members with "
                               "inconsistent domain numbers.");
    }
  }
  return nd;
}

/// Split this function (if needed) into a list of independent functions.
/// The number of functions must be the number of domains this function is
/// working on (== getNumberDomains()). The result of evaluation of the
/// created functions on their domains must be the same as if this function
/// was evaluated on the composition of those domains.
std::vector<IFunction_sptr> CompositeFunction::createEquivalentFunctions() const {
  auto nd = getNumberDomains();
  if (nd == 1) {
    return std::vector<IFunction_sptr>(1, FunctionFactory::Instance().createInitialized(asString()));
  }

  auto nf = nFunctions();
  std::vector<std::vector<IFunction_sptr>> equiv;
  equiv.reserve(nf);
  for (size_t i = 0; i < nf; ++i) {
    equiv.emplace_back(getFunction(i)->createEquivalentFunctions());
  }

  std::vector<IFunction_sptr> funs;
  funs.reserve(nd);
  for (size_t i = 0; i < nd; ++i) {
    auto comp = new CompositeFunction;
    funs.emplace_back(IFunction_sptr(comp));
    for (size_t j = 0; j < nf; ++j) {
      comp->addFunction(equiv[j][i]);
    }
  }
  return funs;
}
size_t CompositeFunction::getAttributeOffset(size_t attributeIndex) const {
  auto funcIndex = m_attributeIndex[attributeIndex];
  return std::distance(std::find(m_attributeIndex.begin(), m_attributeIndex.end(), funcIndex),
                       m_attributeIndex.begin() + attributeIndex);
}

} // namespace Mantid::API
