// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

FunctionMultiDomainPresenter::FunctionMultiDomainPresenter()
{
}

void FunctionMultiDomainPresenter::setFunction(IFunction_sptr fun)
{
}

IFunction_sptr FunctionMultiDomainPresenter::getFitFunction() const
{
  return IFunction_sptr();
}

void FunctionMultiDomainPresenter::setFunctionStr(const std::string & funStr)
{
}

void FunctionMultiDomainPresenter::clear()
{
}

} // namespace API
} // namespace MantidQt
