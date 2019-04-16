// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionSingleDomainPresenter.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;


FunctionSingleDomainPresenter::FunctionSingleDomainPresenter(IFunctionView *view)
{
}

void FunctionSingleDomainPresenter::setFunction(IFunction_sptr fun)
{
}

IFunction_sptr FunctionSingleDomainPresenter::getFitFunction() const
{
  return IFunction_sptr();
}

void FunctionSingleDomainPresenter::setFunctionStr(const std::string & funStr)
{
}

void FunctionSingleDomainPresenter::clear()
{
}

} // namespace API
} // namespace MantidQt
