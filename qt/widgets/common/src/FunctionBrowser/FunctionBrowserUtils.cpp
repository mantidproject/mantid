// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include <boost/lexical_cast.hpp>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

std::pair<QString, QString> splitParameterName(const QString &paramName) {
  QString functionIndex;
  QString parameterName = paramName;
  int j = paramName.lastIndexOf('.');
  if (j > 0) {
    ++j;
    functionIndex = paramName.mid(0, j);
    parameterName = paramName.mid(j);
  }
  return std::make_pair(functionIndex, parameterName);
}

IFunction_sptr getFunctionWithPrefix(const QString &prefix,
                                     const IFunction_sptr &fun) {
  if (prefix.isEmpty() || !fun) {
    return fun;
  }
  auto compFun = boost::dynamic_pointer_cast<CompositeFunction>(fun);
  if (!compFun) {
    throw std::runtime_error("Function " + fun->name() + " is not composite");
  }
  auto j = prefix.indexOf('.');
  if (j < 0) {
    throw std::runtime_error(
        "Error in fit function prefix: " + prefix.toStdString() +
        "\nIt must end with a dot (.)");
  }
  if (j < 2 || prefix[0] != 'f') {
    throw std::runtime_error(
        "Error in fit function prefix: " + prefix.toStdString() +
        "\nIt must start with an 'f' followed by an integer.");
  }
  auto funIndex = prefix.mid(1, j - 1).toInt();
  return getFunctionWithPrefix(prefix.mid(j + 1),
                               compFun->getFunction(funIndex));
}

std::pair<QString, int> splitFunctionPrefix(const QString &prefix) {
  if (prefix.isEmpty())
    return std::make_pair("", -1);
  auto j = prefix.lastIndexOf('.', -2);
  auto parentPrefix = prefix.left(j > 0 ? j + 1 : 0);
  auto funIndex = prefix.mid(j + 2, prefix.size() - j - 3).toInt();
  return std::make_pair(parentPrefix, funIndex);
}

std::pair<QString, std::pair<QString, QString>>
splitConstraintString(const QString &constraint) {
  std::pair<QString, std::pair<QString, QString>> error;
  if (constraint.isEmpty())
    return error;
  QString lowerBoundStr;
  QString upperBoundStr;
  QString paramName;
  Mantid::API::Expression expr;
  expr.parse(constraint.toStdString());
  if (expr.name() != "==") {
    return error;
  }
  double temp;
  if (expr.size() == 3) { // lower < param < upper
    // check that the first and third terms are numbers
    if (!boost::conversion::try_lexical_convert(expr[0].str(), temp) ||
        !boost::conversion::try_lexical_convert(expr[2].str(), temp)) {
      return error;
    }
    if (expr[1].operator_name() == "<" && expr[2].operator_name() == "<") {
      lowerBoundStr = QString::fromStdString(expr[0].str());
      upperBoundStr = QString::fromStdString(expr[2].str());
    } else { // assume that the operators are ">"
      lowerBoundStr = QString::fromStdString(expr[2].str());
      upperBoundStr = QString::fromStdString(expr[0].str());
    }
    paramName = QString::fromStdString(expr[1].str());
  } else if (expr.size() == 2) { // lower < param or param > lower etc
    size_t paramPos = 0;
    // find position of the parameter name in expression
    if (!boost::conversion::try_lexical_convert(expr[1].name(), temp)) {
      paramPos = 1;
    }
    std::string op = expr[1].operator_name();
    if (paramPos == 0) { // parameter goes first
      if (op == "<") {   // param < number
        upperBoundStr = QString::fromStdString(expr[1].str());
      } else { // param > number
        lowerBoundStr = QString::fromStdString(expr[1].str());
      }
      paramName = QString::fromStdString(expr[0].str());
    } else { // parameter is second
      if (!boost::conversion::try_lexical_convert(expr[0].name(), temp)) {
        return error; // error - neither terms are numbers
      }

      if (op == "<") { // number < param
        lowerBoundStr = QString::fromStdString(expr[0].str());
      } else { // number > param
        upperBoundStr = QString::fromStdString(expr[0].str());
      }
      paramName = QString::fromStdString(expr[1].str());
    }
  }
  return std::make_pair(paramName,
                        std::make_pair(lowerBoundStr, upperBoundStr));
}

} // namespace MantidWidgets
} // namespace MantidQt
