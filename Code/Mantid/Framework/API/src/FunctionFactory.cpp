#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/LibraryManager.h"
#include <Poco/StringTokenizer.h>
#include <sstream>

namespace Mantid {
namespace API {

FunctionFactoryImpl::FunctionFactoryImpl()
    : Kernel::DynamicFactory<IFunction>() {
  // we need to make sure the library manager has been loaded before we
  // are constructed so that it is destroyed after us and thus does
  // not close any loaded DLLs with loaded algorithms in them
  Mantid::Kernel::LibraryManager::Instance();
}

FunctionFactoryImpl::~FunctionFactoryImpl() {}

IFunction_sptr
FunctionFactoryImpl::createFunction(const std::string &type) const {
  IFunction_sptr fun = create(type);
  fun->initialize();
  return fun;
}

/**Creates an instance of a function
 * @param input :: An input string which defines the function and initial values
 * for the parameters.
 * Parameters of different functions are separated by ';'. Parameters of the
 * same function
 * are separated by ','. parameterName=value pairs are used to set a parameter
 * value. For each function
 * "name" parameter must be set to a function name. E.g.
 * input = "name=LinearBackground,A0=0,A1=1; name = Gaussian,
 * PeakCentre=10.,Sigma=1"
 * @return A pointer to the created function
 */
IFunction_sptr
FunctionFactoryImpl::createInitialized(const std::string &input) const {
  Expression expr;
  try {
    expr.parse(input);
  } catch (...) {
    inputError(input);
  }

  const Expression &e = expr.bracketsRemoved();
  std::map<std::string, std::string> parentAttributes;
  if (e.name() == ";") {
    IFunction_sptr fun = createComposite(e, parentAttributes);
    if (!fun)
      inputError();
    return fun;
  }

  return createSimple(e, parentAttributes);
}

/**
 * Create a function from an expression.
 * @param expr :: The input expression
 * @param parentAttributes :: An output map filled with the attribute name &
 * values of the parent function
 * @return A pointer to the created function
 */
IFunction_sptr FunctionFactoryImpl::createSimple(
    const Expression &expr,
    std::map<std::string, std::string> &parentAttributes) const {
  if (expr.name() == "=" && expr.size() > 1) {
    return createFunction(expr.terms()[1].name());
  }

  if (expr.name() != "," || expr.size() == 0) {
    inputError(expr.str());
  }

  const std::vector<Expression> &terms = expr.terms();
  std::vector<Expression>::const_iterator term = terms.begin();

  if (term->name() != "=")
    inputError(expr.str());
  if (term->terms()[0].name() != "name" &&
      term->terms()[0].name() != "composite") {
    throw std::invalid_argument(
        "Function name must be defined before its parameters");
  }
  std::string fnName = term->terms()[1].name();

  IFunction_sptr fun = createFunction(fnName);
  for (++term; term != terms.end();
       ++term) { // loop over function's parameters/attributes
    if (term->name() != "=")
      inputError(expr.str());
    std::string parName = term->terms()[0].name();
    std::string parValue = term->terms()[1].str();
    if (fun->hasAttribute(parName)) {
      // set attribute
      if (parValue.size() > 1 && parValue[0] == '"') {
        // remove the double quotes
        parValue = parValue.substr(1, parValue.size() - 2);
      }
      IFunction::Attribute att = fun->getAttribute(parName);
      att.fromString(parValue);
      fun->setAttribute(parName, att);
    } else if (parName.size() >= 10 && parName.substr(0, 10) == "constraint") {
      // or it can be a list of constraints
      addConstraints(fun, (*term)[1]);
    } else if (parName == "ties") {
      addTies(fun, (*term)[1]);
    } else if (!parName.empty() && parName[0] == '$') {
      parName.erase(0, 1);
      parentAttributes[parName] = parValue;
    } else {
      // set initial parameter value
      fun->setParameter(parName, atof(parValue.c_str()));
    }
  } // for term

  fun->applyTies();
  return fun;
}

/**
 * Create a composite function from an expression.
 * @param expr :: The input expression
 * @param parentAttributes :: An output map filled with the attribute name &
 * values of the parent function
 * @return A pointer to the created function
 */
CompositeFunction_sptr FunctionFactoryImpl::createComposite(
    const Expression &expr,
    std::map<std::string, std::string> &parentAttributes) const {
  if (expr.name() != ";")
    inputError(expr.str());

  if (expr.size() == 0) {
    return CompositeFunction_sptr();
  }

  const std::vector<Expression> &terms = expr.terms();
  std::vector<Expression>::const_iterator it = terms.begin();
  const Expression &term = it->bracketsRemoved();

  CompositeFunction_sptr cfun;
  if (term.name() == "=") {
    if (term.terms()[0].name() == "composite") {
      cfun = boost::dynamic_pointer_cast<CompositeFunction>(
          createFunction(term.terms()[1].name()));
      if (!cfun)
        inputError(expr.str());
      ++it;
    } else if (term.terms()[0].name() == "name") {
      cfun = boost::dynamic_pointer_cast<CompositeFunction>(
          createFunction("CompositeFunction"));
      if (!cfun)
        inputError(expr.str());
    } else {
      inputError(expr.str());
    }
  } else if (term.name() == ",") {
    std::vector<Expression>::const_iterator firstTerm = term.terms().begin();
    if (firstTerm->name() == "=") {
      if (firstTerm->terms()[0].name() == "composite") {
        cfun = boost::dynamic_pointer_cast<CompositeFunction>(
            createSimple(term, parentAttributes));
        if (!cfun)
          inputError(expr.str());
        ++it;
      } else if (firstTerm->terms()[0].name() == "name") {
        cfun = boost::dynamic_pointer_cast<CompositeFunction>(
            createFunction("CompositeFunction"));
        if (!cfun)
          inputError(expr.str());
      } else {
        inputError(expr.str());
      }
    }
  } else if (term.name() == ";") {
    cfun = boost::dynamic_pointer_cast<CompositeFunction>(
        createFunction("CompositeFunction"));
    if (!cfun)
      inputError(expr.str());
  } else {
    inputError(expr.str());
  }

  for (; it != terms.end(); ++it) {
    const Expression &term = it->bracketsRemoved();
    IFunction_sptr fun;
    std::map<std::string, std::string> pAttributes;
    if (term.name() == ";") {
      fun = createComposite(term, pAttributes);
      if (!fun)
        continue;
    } else {
      std::string parName = term[0].name();
      if (parName.size() >= 10 && parName.substr(0, 10) == "constraint") {
        addConstraints(cfun, term[1]);
        continue;
      } else if (parName == "ties") {
        addTies(cfun, term[1]);
        continue;
      } else {
        fun = createSimple(term, pAttributes);
      }
    }
    cfun->addFunction(fun);
    size_t i = cfun->nFunctions() - 1;
    for (auto att = pAttributes.begin(); att != pAttributes.end(); ++att) {
      cfun->setLocalAttributeValue(i, att->first, att->second);
    }
  }

  cfun->applyTies();
  return cfun;
}

/// Throw an exception
void FunctionFactoryImpl::inputError(const std::string &str) const {
  std::string msg("Error in input string to FunctionFactory");
  if (!str.empty()) {
    msg += "\n" + str;
  }
  throw std::invalid_argument(msg);
}

/**
 * Add constraints to the created function
 * @param fun :: The function
 * @param expr :: The constraint expression. The expression name must be either
 * a single constraint
 *    expression such as "0 < Sigma < 1" or a list of constraint expressions
 * separated by commas ','
 *    and enclosed in brackets "(...)" .
 */
void FunctionFactoryImpl::addConstraints(IFunction_sptr fun,
                                         const Expression &expr) const {
  if (expr.name() == ",") {
    for (size_t i = 0; i < expr.size(); i++) {
      addConstraint(fun, expr[i]);
    }
  } else {
    addConstraint(fun, expr);
  }
}

/**
 * Add a constraints to the function
 * @param fun :: The function
 * @param expr :: The constraint expression.
 */
void FunctionFactoryImpl::addConstraint(IFunction_sptr fun,
                                        const Expression &expr) const {
  IConstraint *c =
      ConstraintFactory::Instance().createInitialized(fun.get(), expr);
  fun->addConstraint(c);
}

/**
 * @param fun :: The function
 * @param expr :: The tie expression: either parName = TieString or a list
 *   of name = string pairs
 */
void FunctionFactoryImpl::addTies(IFunction_sptr fun,
                                  const Expression &expr) const {
  if (expr.name() == "=") {
    addTie(fun, expr);
  } else if (expr.name() == ",") {
    for (size_t i = 0; i < expr.size(); i++) {
      addTie(fun, expr[i]);
    }
  }
}

/**
 * @param fun :: The function
 * @param expr :: The tie expression: parName = TieString
 */
void FunctionFactoryImpl::addTie(IFunction_sptr fun,
                                 const Expression &expr) const {
  if (expr.size() > 1) { // if size > 2 it is interpreted as setting a tie (last
                         // expr.term) to multiple parameters, e.g
    // f1.alpha = f2.alpha = f3.alpha = f0.beta^2/2
    const std::string value = expr[expr.size() - 1].str();
    for (size_t i = expr.size() - 1; i != 0;) {
      --i;
      fun->tie(expr[i].name(), value);
    }
  }
}

void FunctionFactoryImpl::subscribe(
    const std::string &className, AbstractFactory *pAbstractFactory,
    Kernel::DynamicFactory<IFunction>::SubscribeAction replace) {
  // Clear the cache, then do all the work in the base class method
  m_cachedFunctionNames.clear();
  Kernel::DynamicFactory<IFunction>::subscribe(className, pAbstractFactory,
                                               replace);
}

void FunctionFactoryImpl::unsubscribe(const std::string &className) {
  // Clear the cache, then do all the work in the base class method
  m_cachedFunctionNames.clear();
  Kernel::DynamicFactory<IFunction>::unsubscribe(className);
}

} // namespace API
} // namespace Mantid
