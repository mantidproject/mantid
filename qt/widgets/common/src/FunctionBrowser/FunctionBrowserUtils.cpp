// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace MantidQt::MantidWidgets {

using namespace Mantid::API;

std::pair<std::string, std::string> splitParameterName(std::string const &paramName) {
  QString functionIndex;
  QString initialParamName = QString::fromStdString(paramName);
  QString parameterName = QString::fromStdString(paramName);
  int j = initialParamName.lastIndexOf('.');
  if (j > 0) {
    ++j;
    functionIndex = initialParamName.mid(0, j);
    parameterName = initialParamName.mid(j);
  }
  return std::make_pair(functionIndex.toStdString(), parameterName.toStdString());
}

IFunction_sptr getFunctionWithPrefix(std::string const &prefix, const IFunction_sptr &fun) {
  if (prefix.empty() || !fun) {
    return fun;
  }
  auto compFun = std::dynamic_pointer_cast<CompositeFunction>(fun);
  if (!compFun) {
    throw std::runtime_error("Function " + fun->name() + " is not composite");
  }
  auto const functionPrefix(QString::fromStdString(prefix));
  auto j = functionPrefix.indexOf('.');
  if (j < 0) {
    throw std::runtime_error("Error in fit function prefix: " + prefix + "\nIt must end with a dot (.)");
  }
  if (j < 2 || functionPrefix[0] != 'f') {
    throw std::runtime_error("Error in fit function prefix: " + prefix +
                             "\nIt must start with an 'f' followed by an integer.");
  }
  auto funIndex = functionPrefix.mid(1, j - 1).toInt();
  return getFunctionWithPrefix(functionPrefix.mid(j + 1).toStdString(), compFun->getFunction(funIndex));
}

std::pair<std::string, int> splitFunctionPrefix(std::string const &prefix) {
  if (prefix.empty())
    return std::make_pair("", -1);
  auto const functionPrefix(QString::fromStdString(prefix));
  auto j = functionPrefix.lastIndexOf('.', -2);
  auto parentPrefix = functionPrefix.left(j > 0 ? j + 1 : 0);
  auto funIndex = functionPrefix.mid(j + 2, functionPrefix.size() - j - 3).toInt();
  return std::make_pair(parentPrefix.toStdString(), funIndex);
}

std::pair<std::string, std::pair<std::string, std::string>> splitConstraintString(std::string const &constraint) {
  std::pair<std::string, std::pair<std::string, std::string>> error;
  if (constraint.empty())
    return error;
  std::string lowerBoundStr;
  std::string upperBoundStr;
  std::string paramName;
  Mantid::API::Expression expr;
  expr.parse(constraint);
  if (expr.name() != "==") {
    return error;
  }
  if (expr.size() == 3) { // lower < param < upper
    try {
      // check that the first and third terms are numbers
      boost::lexical_cast<double>(expr[0].str());
      boost::lexical_cast<double>(expr[2].str());
      if (expr[1].operator_name() == "<" && expr[2].operator_name() == "<") {
        lowerBoundStr = expr[0].str();
        upperBoundStr = expr[2].str();
      } else { // assume that the operators are ">"
        lowerBoundStr = expr[2].str();
        upperBoundStr = expr[0].str();
      }
      paramName = expr[1].str();
    } catch (...) { // error in constraint
      return error;
    }
  } else if (expr.size() == 2) { // lower < param or param > lower etc
    size_t paramPos = 0;
    try // find position of the parameter name in expression
    {
      boost::lexical_cast<double>(expr[1].name());
    } catch (...) {
      paramPos = 1;
    }
    std::string op = expr[1].operator_name();
    if (paramPos == 0) { // parameter goes first
      if (op == "<") {   // param < number
        upperBoundStr = expr[1].str();
      } else { // param > number
        lowerBoundStr = expr[1].str();
      }
      paramName = expr[0].str();
    } else { // parameter is second
      try {
        boost::lexical_cast<double>(expr[0].name());
      } catch (...) { // error - neither terms are numbers
        return error;
      }
      if (op == "<") { // number < param
        lowerBoundStr = expr[0].str();
      } else { // number > param
        upperBoundStr = expr[0].str();
      }
      paramName = expr[1].str();
    }
  }
  return std::make_pair(paramName, std::make_pair(lowerBoundStr, upperBoundStr));
}

bool isNumber(std::string const &str) {
  return !str.empty() && str.find_first_not_of("0123456789.-") == std::string::npos;
}

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) { return subString.empty(); }),
                   subStrings.cend());
  return subStrings;
}

std::size_t getFunctionIndexAt(std::string const &parameter, std::size_t const &index) {
  auto const subStrings = splitStringBy(parameter, ".");
  if (index < subStrings.size()) {
    auto const functionIndex = splitStringBy(subStrings[index], "f");
    if (functionIndex.size() == 1 && isNumber(functionIndex[0]))
      return std::stoull(functionIndex[0]);
  }

  throw std::invalid_argument("No function index was found.");
}

} // namespace MantidQt::MantidWidgets
