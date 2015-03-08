//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/UserFunctionMD.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/tokenizer.hpp>

namespace Mantid {
namespace MDAlgorithms {

// Subscribe the function into the factory.
DECLARE_FUNCTION(UserFunctionMD);

/// Default constructor
UserFunctionMD::UserFunctionMD() {
  m_vars.resize(4);
  std::string varNames[] = {"x", "y", "z", "t"};
  m_varNames.assign(varNames, varNames + m_vars.size());
  for (size_t i = 0; i < m_vars.size(); ++i) {
    m_parser.DefineVar(m_varNames[i], &m_vars[i]);
  }
}

/**
 * @return A list of attribute names
 */
std::vector<std::string> UserFunctionMD::getAttributeNames() const {
  return std::vector<std::string>(1, "Formula");
}

/// Has attribute "Formula"
bool UserFunctionMD::hasAttribute(const std::string &attName) const {
  UNUSED_ARG(attName);
  return attName == "Formula";
}

/// Return Formula
UserFunctionMD::Attribute
UserFunctionMD::getAttribute(const std::string &attName) const {
  UNUSED_ARG(attName);
  return Attribute(m_formula);
}

/** Set Formula
 * @param attName :: Attribute name - must be "Formula"
 * @param attr :: Attribute value - the formula
 */
void UserFunctionMD::setAttribute(const std::string &attName,
                                  const UserFunctionMD::Attribute &attr) {
  UNUSED_ARG(attName);
  m_formula = attr.asString();
  if (!m_vars.empty()) {
    setFormula();
  }
}
/**
* Defining function's parameters here, ie after the workspace is set and
* the dimensions are known.
*/
void UserFunctionMD::initDimensions() {
  // if (!getWorkspace()) return;
  if (m_vars.size() > 4) {
    m_vars.resize(m_dimensionIndexMap.size());
    m_varNames.resize(m_dimensionIndexMap.size());
    for (size_t i = 0; i < m_vars.size(); ++i) {
      m_varNames[i] = "x" + boost::lexical_cast<std::string>(i);
      m_parser.DefineVar(m_varNames[i], &m_vars[i]);
    }
  }
  setFormula();
}

/**
 * Evaluate the function at MD iterator r.
 * @param r :: MD iterator.
 */
double UserFunctionMD::functionMD(const API::IMDIterator &r) const {
  size_t n = m_dimensions.size();
  Kernel::VMD center = r.getCenter();
  double val = 0.0;
  PARALLEL_CRITICAL(function) {
    for (size_t i = 0; i < n; ++i) {
      m_vars[i] = center[i];
    }
    // std::cerr << m_vars[0] << ',' << m_vars[1] << ' ' << m_parser.Eval() <<
    // std::endl;
    try {
      val = m_parser.Eval();
    } catch (mu::Parser::exception_type &e) {
      std::cerr << "Message:  " << e.GetMsg() << "\n";
      std::cerr << "Formula:  " << e.GetExpr() << "\n";
      std::cerr << "Token:    " << e.GetToken() << "\n";
      std::cerr << "Position: " << e.GetPos() << "\n";
      std::cerr << "Errc:     " << e.GetCode() << "\n";
      throw;
    }
  }
  return val;
}
/** Static callback function used by MuParser to initialize variables implicitly
@param varName :: The name of a new variable
@param pufun :: Pointer to the function
*/
double *UserFunctionMD::AddVariable(const char *varName, void *pufun) {
  UserFunctionMD &fun = *reinterpret_cast<UserFunctionMD *>(pufun);

  std::vector<std::string>::iterator x =
      std::find(fun.m_varNames.begin(), fun.m_varNames.end(), varName);
  if (x != fun.m_varNames.end()) {
    // std::vector<std::string>::difference_type i =
    // std::distance(fun.m_varNames.begin(),x);
    throw std::runtime_error("UserFunctionMD variables are not defined");
  } else {
    try {
      fun.declareParameter(varName, 0.0);
    } catch (...) {
    }
  }

  // The returned pointer will never be used. Just returning a valid double
  // pointer
  return &fun.m_vars[0];
}

/**
* Initializes the mu::Parser.
*/
void UserFunctionMD::setFormula() {
  // variables must be already defined
  if (m_vars.empty())
    return;
  if (m_formula.empty()) {
    m_formula = "0";
  }
  m_parser.SetVarFactory(AddVariable, this);
  m_parser.SetExpr(m_formula);
  // declare function parameters using mu::Parser's implicit variable setting
  m_parser.Eval();
  m_parser.ClearVar();
  // set muParser variables
  for (size_t i = 0; i < m_vars.size(); ++i) {
    m_parser.DefineVar(m_varNames[i], &m_vars[i]);
  }
  for (size_t i = 0; i < nParams(); i++) {
    m_parser.DefineVar(parameterName(i), getParameterAddress(i));
  }

  m_parser.SetExpr(m_formula);
}

} // namespace MDAlgorithms
} // namespace Mantid
