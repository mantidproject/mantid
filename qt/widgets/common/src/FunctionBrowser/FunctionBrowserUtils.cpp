// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidAPI/CompositeFunction.h"

namespace MantidQt {
namespace MantidWidgets {

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

IFunction_sptr getFunctionWithPrefix(const QString & prefix, const IFunction_sptr &fun)
{
  if (prefix.isEmpty() || !fun) {
    return fun;
  }
  auto compFun = boost::dynamic_pointer_cast<CompositeFunction>(fun);
  if (!compFun) {
    throw std::runtime_error("Function " + fun->name() + " is not composite");
  }
  auto j = prefix.indexOf('.');
  if (j < 0) {
    throw std::runtime_error("Error in fit function prefix: " + prefix.toStdString() + "\nIt must end with a dot (.)");
  }
  if (j < 2 || prefix[0] != 'f') {
    throw std::runtime_error("Error in fit function prefix: " + prefix.toStdString() + "\nIt must start with an 'f' followed by an integer.");
  }
  auto funIndex = prefix.mid(1, j - 1).toInt();
  return getFunctionWithPrefix(prefix.mid(j + 1), compFun->getFunction(funIndex));
}

} // namespace MantidWidgets
} // namespace MantidQt

