// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/StringTokenizer.h"
#include <boost/lexical_cast.hpp>
#include <sstream>

namespace Mantid::API {

FunctionFactoryImpl::FunctionFactoryImpl() : Kernel::DynamicFactory<IFunction>() {
  // we need to make sure the library manager has been loaded before we
  // are constructed so that it is destroyed after us and thus does
  // not close any loaded DLLs with loaded algorithms in them
  Mantid::Kernel::LibraryManager::Instance();
}

IFunction_sptr FunctionFactoryImpl::createFunction(const std::string &type) const {
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
IFunction_sptr FunctionFactoryImpl::createInitialized(const std::string &input) const {
  Expression expr;
  try {
    expr.parse(input);
  } catch (Expression::ParsingError &e) {
    inputError(input + "\n    " + e.what());
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
 * @param input :: An input string which defines the function and initial values
 * for the parameters.
 * Parameters of different functions are separated by ';'. Parameters of the
 * same function
 * are separated by ','. parameterName=value pairs are used to set a parameter
 * value. For each function
 * "name" parameter must be set to a function name. E.g.
 * input = "name=LinearBackground,A0=0,A1=1; name = Gaussian,
 * PeakCentre=10.,Sigma=1"
 * @param domainNumber :: The number of domains to add to the function.
 * @return A pointer to the created function.
 */
std::shared_ptr<MultiDomainFunction>
FunctionFactoryImpl::createInitializedMultiDomainFunction(const std::string &input, size_t domainNumber) const {
  auto singleFunction = createInitialized(input);
  auto multiDomainFunction = std::make_shared<MultiDomainFunction>();

  if (!singleFunction) {
    return multiDomainFunction;
  }

  for (size_t i = 0; i < domainNumber; ++i) {
    multiDomainFunction->addFunction(singleFunction->clone());
    multiDomainFunction->setDomainIndex(i, i);
  }

  return multiDomainFunction;
}

/**
 * Create a function from an expression.
 * @param expr :: The input expression
 * @param parentAttributes :: An output map filled with the attribute name &
 * values of the parent function
 * @return A pointer to the created function
 */
IFunction_sptr FunctionFactoryImpl::createSimple(const Expression &expr,
                                                 std::map<std::string, std::string> &parentAttributes) const {
  if (expr.name() == "=" && expr.size() > 1) {
    return createFunction(expr.terms()[1].name());
  }

  if (expr.name() != "," || expr.size() == 0) {
    inputError(expr.str());
  }

  const std::vector<Expression> &terms = expr.terms();
  auto term = terms.cbegin();

  if (term->name() != "=")
    inputError(expr.str());
  if (term->terms()[0].name() != "name" && term->terms()[0].name() != "composite") {
    throw std::invalid_argument("Function name must be defined before its parameters");
  }
  std::string fnName = term->terms()[1].name();

  IFunction_sptr fun = createFunction(fnName);
  for (++term; term != terms.end(); ++term) { // loop over function's parameters/attributes
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
      addConstraints(fun, (*term)[1].bracketsRemoved());
    } else if (parName == "ties") {
      addTies(fun, (*term)[1].bracketsRemoved());
    } else if (!parName.empty() && parName[0] == '$') {
      parName.erase(0, 1);
      parentAttributes[parName] = parValue;
    } else {
      // set initial parameter value
      try {
        fun->setParameter(parName, boost::lexical_cast<double>(parValue));
      } catch (boost::bad_lexical_cast &) {
        throw std::runtime_error(std::string("Error in value of parameter ")
                                     .append(parName)
                                     .append(".\n")
                                     .append(parValue)
                                     .append(" cannot be interpreted as a floating point value."));
      }
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
CompositeFunction_sptr
FunctionFactoryImpl::createComposite(const Expression &expr,
                                     std::map<std::string, std::string> &parentAttributes) const {
  if (expr.name() != ";")
    inputError(expr.str());

  if (expr.size() == 0) {
    return CompositeFunction_sptr();
  }

  const std::vector<Expression> &terms = expr.terms();
  auto it = terms.cbegin();
  const Expression &term = it->bracketsRemoved();

  CompositeFunction_sptr cfun;
  if (term.name() == "=") {
    if (term.terms()[0].name() == "composite") {
      cfun = std::dynamic_pointer_cast<CompositeFunction>(createFunction(term.terms()[1].name()));
      if (!cfun)
        inputError(expr.str());
      ++it;
    } else if (term.terms()[0].name() == "name") {
      cfun = std::dynamic_pointer_cast<CompositeFunction>(createFunction("CompositeFunction"));
      if (!cfun)
        inputError(expr.str());
    } else {
      inputError(expr.str());
    }
  } else if (term.name() == ",") {
    auto firstTerm = term.terms().cbegin();
    if (firstTerm->name() == "=") {
      if (firstTerm->terms()[0].name() == "composite") {
        cfun = std::dynamic_pointer_cast<CompositeFunction>(createSimple(term, parentAttributes));
        if (!cfun)
          inputError(expr.str());
        ++it;
      } else if (firstTerm->terms()[0].name() == "name") {
        cfun = std::dynamic_pointer_cast<CompositeFunction>(createFunction("CompositeFunction"));
        if (!cfun)
          inputError(expr.str());
      } else {
        inputError(expr.str());
      }
    }
  } else if (term.name() == ";") {
    cfun = std::dynamic_pointer_cast<CompositeFunction>(createFunction("CompositeFunction"));
    if (!cfun)
      inputError(expr.str());
  } else {
    inputError(expr.str());
  }

  if (!cfun)
    inputError(expr.str());

  for (; it != terms.end(); ++it) {
    const Expression &currentTerm = it->bracketsRemoved();
    IFunction_sptr fun;
    std::map<std::string, std::string> pAttributes;
    if (currentTerm.name() == ";") {
      fun = createComposite(currentTerm, pAttributes);
      if (!fun)
        continue;
    } else {
      std::string parName = currentTerm[0].name();
      if (parName.size() >= 10 && parName.substr(0, 10) == "constraint") {
        addConstraints(cfun, currentTerm[1].bracketsRemoved());
        continue;
      } else if (parName == "ties") {
        addTies(cfun, currentTerm[1].bracketsRemoved());
        continue;
      } else {
        fun = createSimple(currentTerm, pAttributes);
      }
    }
    cfun->addFunction(fun);
    size_t i = cfun->nFunctions() - 1;
    for (auto &pAttribute : pAttributes) {
      // Apply parent attributes of the child function to this function. If this
      // function doesn't have those attributes, they get passed up the chain to
      // this function's parent.
      if (cfun->hasLocalAttribute(pAttribute.first)) {
        cfun->setLocalAttributeValue(i, pAttribute.first, pAttribute.second);
      } else {
        parentAttributes[pAttribute.first] = pAttribute.second;
      }
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
void FunctionFactoryImpl::addConstraints(const IFunction_sptr &fun, const Expression &expr) const {
  if (expr.name() == ",") {
    for (auto it = expr.begin(); it != expr.end(); ++it) {
      // If this is a penalty term, we used it on the previous iteration
      // so we move on to the next term.
      auto constraint = (*it);
      std::string constraint_term = constraint.terms()[0].str();
      if (constraint_term.compare("penalty") == 0) {
        continue;
      }

      if ((it + 1) != expr.end()) {
        auto next_constraint = *(it + 1);
        std::string next_term = next_constraint.terms()[0].str();
        if (next_term.compare("penalty") == 0) {
          addConstraint(fun, constraint, next_constraint);
        } else {
          addConstraint(fun, constraint);
        }
      } else {
        addConstraint(fun, constraint);
      }
    }
  } else { // There was a single constraint given, cannot contain a penalty
    addConstraint(fun, expr);
  }
}

/**
 * Add a constraints to the function
 * @param fun :: The function
 * @param expr :: The constraint expression.
 */
void FunctionFactoryImpl::addConstraint(const std::shared_ptr<IFunction> &fun, const Expression &expr) const {
  auto c = std::unique_ptr<IConstraint>(ConstraintFactory::Instance().createInitialized(fun.get(), expr));
  c->setPenaltyFactor(c->getDefaultPenaltyFactor());
  fun->addConstraint(std::move(c));
}

/**
 * Add a constraint to the function with non-default penalty
 * @param fun :: The function
 * @param constraint_expr :: The constraint expression.
 * @param penalty_expr :: The penalty expression.
 */
void FunctionFactoryImpl::addConstraint(const std::shared_ptr<IFunction> &fun, const Expression &constraint_expr,
                                        const Expression &penalty_expr) const {
  auto c = std::unique_ptr<IConstraint>(ConstraintFactory::Instance().createInitialized(fun.get(), constraint_expr));
  double penalty_factor = std::stof(penalty_expr.terms()[1].str(), NULL);
  c->setPenaltyFactor(penalty_factor);
  fun->addConstraint(std::move(c));
}

/**
 * @param fun :: The function
 * @param expr :: The tie expression: either parName = TieString or a list
 *   of name = string pairs
 */
void FunctionFactoryImpl::addTies(const IFunction_sptr &fun, const Expression &expr) const {
  if (expr.name() == "=") {
    addTie(fun, expr);
  } else if (expr.name() == ",") {
    for (const auto &constraint : expr) {
      addTie(fun, constraint);
    }
  }
}

/**
 * @param fun :: The function
 * @param expr :: The tie expression: parName = TieString
 */
void FunctionFactoryImpl::addTie(const IFunction_sptr &fun, const Expression &expr) const {
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

std::vector<std::string> FunctionFactoryImpl::getFunctionNamesGUI() const {
  auto allNames = getFunctionNames<IFunction1D>();
  auto ImmutableCompositeFunctions = getFunctionNames<ImmutableCompositeFunction>();
  allNames.insert(allNames.end(), ImmutableCompositeFunctions.begin(), ImmutableCompositeFunctions.end());
  allNames.emplace_back("ProductFunction");
  allNames.emplace_back("CompositeFunction");
  allNames.emplace_back("Convolution");
  std::sort(allNames.begin(), allNames.end());
  std::vector<std::string> names;
  names.reserve(allNames.size());
  auto excludes = Kernel::ConfigService::Instance().getString("curvefitting.guiExclude");
  Kernel::StringTokenizer tokenizer(excludes, ";", Kernel::StringTokenizer::TOK_TRIM);
  std::set<std::string> excludeList(tokenizer.begin(), tokenizer.end());
  std::copy_if(allNames.cbegin(), allNames.cend(), std::back_inserter(names),
               [&excludeList](const auto &name) { return excludeList.count(name) == 0; });
  return names;
}

void FunctionFactoryImpl::subscribe(const std::string &className, std::unique_ptr<AbstractFactory> pAbstractFactory,
                                    Kernel::DynamicFactory<IFunction>::SubscribeAction replace) {
  // Clear the cache, then do all the work in the base class method
  m_cachedFunctionNames.clear();
  Kernel::DynamicFactory<IFunction>::subscribe(className, std::move(pAbstractFactory), replace);
}

void FunctionFactoryImpl::unsubscribe(const std::string &className) {
  // Clear the cache, then do all the work in the base class method
  m_cachedFunctionNames.clear();
  Kernel::DynamicFactory<IFunction>::unsubscribe(className);
}

} // namespace Mantid::API
