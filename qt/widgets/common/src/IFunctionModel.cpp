// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/IFunctionModel.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

namespace MantidQt::MantidWidgets {

using namespace Mantid::API;

void IFunctionModel::setFunctionString(std::string const &funStr) {
  if (funStr.empty()) {
    clear();
    return;
  }
  setFunction(FunctionFactory::Instance().createInitialized(funStr));
}

std::string IFunctionModel::getFunctionString() const {
  auto fun = getCurrentFunction();
  if (!fun)
    return "";
  return fun->asString();
}

std::string IFunctionModel::getFitFunctionString() const {
  auto fun = getFitFunction();
  if (!fun)
    return "";
  return fun->asString();
}

void IFunctionModel::clear() { setFunction(IFunction_sptr()); }

int IFunctionModel::getNumberLocalFunctions() const {
  auto const n = getNumberDomains();
  return n > 0 ? n : 1;
}

void IFunctionModel::copyParametersAndErrors(const IFunction &funFrom, IFunction &funTo) {
  if (funTo.nParams() != funFrom.nParams())
    return;
  for (size_t i = 0; i < funFrom.nParams(); ++i) {
    funTo.setParameter(i, funFrom.getParameter(i));
    funTo.setError(i, funFrom.getError(i));
  }
}

void IFunctionModel::copyParametersAndErrorsToAllLocalFunctions(const IFunction &fun) {
  for (auto i = 0; i < getNumberLocalFunctions(); ++i) {
    auto localFun = getSingleFunction(i);
    copyParametersAndErrors(fun, *localFun);
  }
}

} // namespace MantidQt::MantidWidgets
