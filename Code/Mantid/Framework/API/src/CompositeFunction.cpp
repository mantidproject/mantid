//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Mantid {
namespace API {

namespace {
/// static logger
Kernel::Logger g_log("CompositeFunction");
}

using std::size_t;

DECLARE_FUNCTION(CompositeFunction)

/// Default constructor
CompositeFunction::CompositeFunction() : IFunction(), m_nParams(0) {
  declareAttribute("NumDeriv", Attribute(false));
}

/// Destructor
CompositeFunction::~CompositeFunction() {}

/// Function initialization. Declare function parameters in this method.
void CompositeFunction::init() {}

/**
 * Writes itself into a string. Functions derived from CompositeFunction must
 * override this method with something like this:
 *   std::string NewFunction::asString()const
 *   {
 *      ostr << "composite=" << this->name() << ';';
 *      // write NewFunction's own attributes and parameters
 *      ostr << CompositeFunction::asString();
 *      // write NewFunction's own ties and constraints
 *      // ostr << ";constraints=(" << ... <<")";
 *   }
 * @return the string representation of the composite function
 */
std::string CompositeFunction::asString() const {
  std::ostringstream ostr;

  // if empty just return function name
  if (nFunctions() == 0) {
    return "name=" + name();
  }

  if (name() != "CompositeFunction" || nAttributes() > 1 ||
      getAttribute("NumDeriv").asBool() == true) {
    ostr << "composite=" << name();
    std::vector<std::string> attr = this->getAttributeNames();
    for (size_t i = 0; i < attr.size(); i++) {
      std::string attName = attr[i];
      std::string attValue = this->getAttribute(attr[i]).value();
      if (!attValue.empty()) {
        ostr << ',' << attName << '=' << attValue;
      }
    }
    ostr << ';';
  }
  for (size_t i = 0; i < nFunctions(); i++) {
    IFunction_sptr fun = getFunction(i);
    bool isComp = boost::dynamic_pointer_cast<CompositeFunction>(fun) != 0;
    if (isComp)
      ostr << '(';
    ostr << fun->asString();
    if (isComp)
      ostr << ')';
    if (i < nFunctions() - 1) {
      ostr << ';';
    }
  }
  std::string ties;
  for (size_t i = 0; i < nParams(); i++) {
    const ParameterTie *tie = getTie(i);
    if (tie) {
      IFunction_sptr fun = getFunction(functionIndex(i));
      std::string tmp = tie->asString(fun.get());
      if (tmp.empty()) {
        tmp = tie->asString(this);
        if (!tmp.empty()) {
          if (!ties.empty()) {
            ties += ",";
          }
          ties += tmp;
        }
      }
    }
  }
  if (!ties.empty()) {
    ostr << ";ties=(" << ties << ")";
  }
  return ostr.str();
}

/**
 * @param ws A pointer to the workspace being fitted
 */
void CompositeFunction::setWorkspace(boost::shared_ptr<const Workspace> ws) {
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
void CompositeFunction::setMatrixWorkspace(
    boost::shared_ptr<const MatrixWorkspace> workspace, size_t wi,
    double startX, double endX) {
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    m_functions[iFun]->setMatrixWorkspace(workspace, wi, startX, endX);
  }
}

/** Function you want to fit to.
 *  @param domain :: An instance of FunctionDomain with the function arguments.
 *  @param values :: A FunctionValues instance for storing the calculated
 * values.
 */
void CompositeFunction::function(const FunctionDomain &domain,
                                 FunctionValues &values) const {
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
void CompositeFunction::functionDeriv(const FunctionDomain &domain,
                                      Jacobian &jacobian) {
  if (getAttribute("NumDeriv").asBool()) {
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
void CompositeFunction::setParameter(size_t i, const double &value,
                                     bool explicitlySet) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setParameter(i - m_paramOffsets[iFun], value,
                                  explicitlySet);
}

/** Sets a new description to the i-th parameter.
 *  @param i :: The parameter index
 *  @param description :: The new description
 */
void
CompositeFunction::setParameterDescription(size_t i,
                                           const std::string &description) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setParameterDescription(i - m_paramOffsets[iFun],
                                             description);
}

/** Get the i-th parameter.
 *  @param i :: The parameter index
 *  @return value of the requested parameter
 */
double CompositeFunction::getParameter(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->getParameter(i - m_paramOffsets[iFun]);
}

/**
 * Sets a new value to a parameter by name.
 * @param name :: The name of the parameter.
 * @param value :: The new value
 * @param explicitlySet :: A boolean falgging the parameter as explicitly set
 * (by user)
 */
void CompositeFunction::setParameter(const std::string &name,
                                     const double &value, bool explicitlySet) {
  std::string pname;
  size_t index;
  parseName(name, index, pname);
  getFunction(index)->setParameter(pname, value, explicitlySet);
}

/**
 * Sets a new description to a parameter by name.
 * @param name :: The name of the parameter.
 * @param description :: The new description
 */
void
CompositeFunction::setParameterDescription(const std::string &name,
                                           const std::string &description) {
  std::string pname;
  size_t index;
  parseName(name, index, pname);
  getFunction(index)->setParameterDescription(pname, description);
}

/**
 * Parameters by name.
 * @param name :: The name of the parameter.
 * @return value of the requested named parameter
 */
double CompositeFunction::getParameter(const std::string &name) const {
  std::string pname;
  size_t index;
  parseName(name, index, pname);
  return getFunction(index)->getParameter(pname);
}

/// Total number of parameters
size_t CompositeFunction::nParams() const { return m_nParams; }

/**
 *
 * @param name :: The name of a parameter
 * @return index of the requested named parameter
 */
size_t CompositeFunction::parameterIndex(const std::string &name) const {
  std::string pname;
  size_t index;
  parseName(name, index, pname);
  return getFunction(index)->parameterIndex(pname) + m_paramOffsets[index];
}

/// Returns the name of parameter
/// @param i :: The index
/// @return The name of the parameter
std::string CompositeFunction::parameterName(size_t i) const {
  size_t iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.'
       << m_functions[iFun]->parameterName(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/// Returns the description of parameter
/// @param i :: The index
/// @return The description of the parameter
std::string CompositeFunction::parameterDescription(size_t i) const {
  size_t iFun = functionIndex(i);
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
 * Set the fitting error for a parameter
 * @param i :: The index of a parameter
 * @param err :: The error value to set
 */
void CompositeFunction::setError(size_t i, double err) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setError(i - m_paramOffsets[iFun], err);
}

/// Value of i-th active parameter. Override this method to make fitted
/// parameters different from the declared
double CompositeFunction::activeParameter(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->activeParameter(i - m_paramOffsets[iFun]);
}

/// Set new value of i-th active parameter. Override this method to make fitted
/// parameters different from the declared
void CompositeFunction::setActiveParameter(size_t i, double value) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->setActiveParameter(i - m_paramOffsets[iFun], value);
}

/// Returns the name of active parameter i
std::string CompositeFunction::nameOfActive(size_t i) const {
  size_t iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.'
       << m_functions[iFun]->nameOfActive(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/// Returns the description of active parameter i
std::string CompositeFunction::descriptionOfActive(size_t i) const {
  size_t iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << m_functions[iFun]->descriptionOfActive(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/**
 * query to see in the function is active
 * @param i :: The index of a declared parameter
 * @return true if parameter i is active
 */
bool CompositeFunction::isActive(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->isActive(i - m_paramOffsets[iFun]);
}

/**
 * query to see in the function is active
 * @param i :: The index of a declared parameter
 * @return true if parameter i is active
 */
bool CompositeFunction::isFixed(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->isFixed(i - m_paramOffsets[iFun]);
}

/**
 * @param i :: A declared parameter index to be removed from active
 */
void CompositeFunction::fix(size_t i) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->fix(i - m_paramOffsets[iFun]);
}

/** Makes a parameter active again. It doesn't change the parameter's tie.
 * @param i :: A declared parameter index to be restored to active
 */
void CompositeFunction::unfix(size_t i) {
  size_t iFun = functionIndex(i);
  m_functions[iFun]->unfix(i - m_paramOffsets[iFun]);
}

/** Makes sure that the function is consistent.
 */
void CompositeFunction::checkFunction() {
  m_nParams = 0;
  m_paramOffsets.clear();
  m_IFunction.clear();

  std::vector<IFunction_sptr> functions(m_functions.begin(), m_functions.end());
  m_functions.clear();

  for (std::vector<IFunction_sptr>::size_type i = 0; i < functions.size();
       i++) {
    IFunction_sptr f = functions[i];
    CompositeFunction_sptr cf =
        boost::dynamic_pointer_cast<CompositeFunction>(f);
    if (cf)
      cf->checkFunction();
    addFunction(f);
  }
}

/** Add a function
 * @param f :: A pointer to the added function
 * @return The function index
 */
size_t CompositeFunction::addFunction(IFunction_sptr f) {
  m_IFunction.insert(m_IFunction.end(), f->nParams(), m_functions.size());
  m_functions.push_back(f);
  //?f->init();
  if (m_paramOffsets.size() == 0) {
    m_paramOffsets.push_back(0);
    m_nParams = f->nParams();
  } else {
    m_paramOffsets.push_back(m_nParams);
    m_nParams += f->nParams();
  }
  return m_functions.size() - 1;
}

/** Remove a function
 * @param i :: The index of the function to remove
 */
void CompositeFunction::removeFunction(size_t i) {
  if (i >= nFunctions())
    throw std::out_of_range("Function index out of range.");

  IFunction_sptr fun = getFunction(i);

  size_t dnp = fun->nParams();

  for (size_t j = 0; j < nParams();) {
    ParameterTie *tie = getTie(j);
    if (tie && tie->findParametersOf(fun.get())) {
      removeTie(j);
    } else {
      j++;
    }
  }

  // Shift down the function indeces for parameters
  for (std::vector<size_t>::iterator it = m_IFunction.begin();
       it != m_IFunction.end();) {

    if (*it == i) {
      it = m_IFunction.erase(it);
    } else {
      if (*it > i) {
        *it -= 1;
      }
      ++it;
    }
  }

  m_nParams -= dnp;
  // Shift the parameter offsets down by the total number of i-th function's
  // params
  for (size_t j = i + 1; j < nFunctions(); j++) {
    m_paramOffsets[j] -= dnp;
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
void CompositeFunction::replaceFunctionPtr(const IFunction_sptr f_old,
                                           IFunction_sptr f_new) {
  std::vector<IFunction_sptr>::const_iterator it =
      std::find(m_functions.begin(), m_functions.end(), f_old);
  if (it == m_functions.end())
    return;
  std::vector<IFunction_sptr>::difference_type iFun = it - m_functions.begin();
  replaceFunction(iFun, f_new);
}

/** Replace a function with a new one. The old function is deleted.
 * @param i :: The index of the function to replace
 * @param f :: A pointer to the new function
 */
void CompositeFunction::replaceFunction(size_t i, IFunction_sptr f) {
  if (i >= nFunctions())
    throw std::out_of_range("Function index out of range.");

  IFunction_sptr fun = getFunction(i);
  size_t np_old = fun->nParams();

  size_t np_new = f->nParams();

  // Modify function indeces: The new function may have different number of
  // parameters
  {
    std::vector<size_t>::iterator itFun =
        std::find(m_IFunction.begin(), m_IFunction.end(), i);
    if (itFun != m_IFunction.end()) // functions must have at least 1 parameter
    {
      if (np_old > np_new) {
        m_IFunction.erase(itFun, itFun + np_old - np_new);
      } else if (np_old < np_new) {
        m_IFunction.insert(itFun, np_new - np_old, i);
      }
    } else if (np_new > 0) // it could happen if the old function is an empty
                           // CompositeFunction
    {
      itFun = std::find_if(m_IFunction.begin(), m_IFunction.end(),
                           std::bind2nd(std::greater<size_t>(), i));
      m_IFunction.insert(itFun, np_new, i);
    }
  }

  size_t dnp = np_new - np_old;
  m_nParams += dnp;
  // Shift the parameter offsets down by the total number of i-th function's
  // params
  for (size_t j = i + 1; j < nFunctions(); j++) {
    m_paramOffsets[j] += dnp;
  }

  m_functions[i] = f;
}

/**
 * @param i :: The index of the function
 * @return function at the requested index
 */
IFunction_sptr CompositeFunction::getFunction(std::size_t i) const {
  if (i >= nFunctions()) {
    throw std::out_of_range("Function index out of range.");
  }
  return m_functions[i];
}

/**
 * Get the index of the function to which parameter i belongs
 * @param i :: The parameter index
 * @return function index of the requested parameter
 */
size_t CompositeFunction::functionIndex(std::size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("Function parameter index out of range.");
  }
  return m_IFunction[i];
}

/**
* @param varName :: The variable name which may contain function index (
* [f<index.>]name )
* @param index :: Receives function index or throws std::invalid_argument
* @param name :: Receives the parameter name
*/
void CompositeFunction::parseName(const std::string &varName, size_t &index,
                                  std::string &name) {
  size_t i = varName.find('.');
  if (i == std::string::npos) {
    throw std::invalid_argument("Parameter " + varName + " not found.");
  } else {
    if (varName[0] != 'f')
      throw std::invalid_argument(
          "External function parameter name must start with 'f'");

    std::string sindex = varName.substr(1, i - 1);
    index = boost::lexical_cast<int>(sindex);

    if (i == varName.size() - 1)
      throw std::invalid_argument("Name cannot be empty");

    name = varName.substr(i + 1);
  }
}

/** Returns the index of parameter i as it declared in its function
 * @param i :: The parameter index
 * @return The local index of the parameter
 */
size_t CompositeFunction::parameterLocalIndex(size_t i) const {
  size_t iFun = functionIndex(i);
  return i - m_paramOffsets[iFun];
}

/** Returns the name of parameter i as it declared in its function
 * @param i :: The parameter index
 * @return The pure parameter name (without the function identifier f#.)
 */
std::string CompositeFunction::parameterLocalName(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->parameterName(i - m_paramOffsets[iFun]);
}

/**
 * Apply the ties.
 */
void CompositeFunction::applyTies() {
  for (size_t i = 0; i < nFunctions(); i++) {
    getFunction(i)->applyTies();
  }
}

/**
 * Clear the ties.
 */
void CompositeFunction::clearTies() {
  for (size_t i = 0; i < nFunctions(); i++) {
    getFunction(i)->clearTies();
  }
}

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i :: The index of the tied parameter.
 * @return True if successfull
 */
bool CompositeFunction::removeTie(size_t i) {
  size_t iFun = functionIndex(i);
  bool res = m_functions[iFun]->removeTie(i - m_paramOffsets[iFun]);
  return res;
}

/** Get the tie of i-th parameter
 * @param i :: The parameter index
 * @return A pointer to the tie.
 */
ParameterTie *CompositeFunction::getTie(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->getTie(i - m_paramOffsets[iFun]);
}

/**
 * Attaches a tie to this function. The attached tie is owned by the function.
 * @param tie :: A pointer to a new tie
 */
void CompositeFunction::addTie(ParameterTie *tie) {
  size_t i = getParameterIndex(*tie);
  size_t iFun = functionIndex(i);
  m_functions[iFun]->addTie(tie);
}

/**
 * Declare a new parameter. To used in the implementation'c constructor.
 * @param name :: The parameter name.
 * @param initValue :: The initial value for the parameter
 * @param description :: Parameter documentation
 */
void CompositeFunction::declareParameter(const std::string &name,
                                         double initValue,
                                         const std::string &description) {
  (void)name;        // Avoid compiler warning
  (void)initValue;   // Avoid compiler warning
  (void)description; // Avoid compiler warning
  throw Kernel::Exception::NotImplementedError(
      "CompositeFunction cannot not have its own parameters.");
}

/** Add a constraint
 *  @param ic :: Pointer to a constraint.
 */
void CompositeFunction::addConstraint(IConstraint *ic) {
  size_t i = getParameterIndex(*ic);
  size_t iFun = functionIndex(i);
  getFunction(iFun)->addConstraint(ic);
}

/**
 * Prepare the function for a fit.
 */
void CompositeFunction::setUpForFit() {
  // set up the member functions
  for (size_t i = 0; i < nFunctions(); i++) {
    getFunction(i)->setUpForFit();
  }
  // unfortuately the code below breaks some system tests (IRISFuryAndFuryFit)
  // it looks as if using numeric derivatives can give different fit results
  // to fit with analytical ones
  //
  // if parameters have non-constant ties enable numerical derivatives
  // for(size_t i = 0; i < nParams(); ++i)
  //{
  //  ParameterTie* tie = getTie( i );
  //  if ( tie && !tie->isConstant() )
  //  {
  //    useNumericDerivatives( true );
  //    break;
  //  }
  //}

  // instead of automatically switching to numeric derivatives
  // log a warning about a danger of not using it
  if (!getAttribute("NumDeriv").asBool()) {
    for (size_t i = 0; i < nParams(); ++i) {
      ParameterTie *tie = getTie(i);
      if (tie && !tie->isConstant()) {
        g_log.warning() << "Numeric derivatives should be used when "
                           "non-constant ties defined." << std::endl;
        break;
      }
    }
  }
}

/// Get constraint
/// @param i :: the index
/// @return A pointer to the constraint
IConstraint *CompositeFunction::getConstraint(size_t i) const {
  size_t iFun = functionIndex(i);
  return m_functions[iFun]->getConstraint(i - m_paramOffsets[iFun]);
}

/** Remove a constraint
 * @param parName :: The name of a parameter which constarint to remove.
 */
void CompositeFunction::removeConstraint(const std::string &parName) {
  size_t iPar = parameterIndex(parName);
  size_t iFun = functionIndex(iPar);
  getFunction(iFun)->removeConstraint(parameterLocalName(iPar));
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
size_t
CompositeFunction::getParameterIndex(const ParameterReference &ref) const {
  if (ref.getFunction() == this && ref.getIndex() < nParams()) {
    return ref.getIndex();
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
 * Returns the shrared pointer to the function conataining a parameter
 * @param ref :: The reference
 * @return A function containing parameter pointed to by ref
 */
IFunction_sptr
CompositeFunction::getContainingFunction(const ParameterReference &ref) const {
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    IFunction_sptr fun = getFunction(iFun);
    if (fun->getParameterIndex(ref) < fun->nParams()) {
      return fun;
    }
  }
  return IFunction_sptr();
}

} // namespace API
} // namespace Mantid
