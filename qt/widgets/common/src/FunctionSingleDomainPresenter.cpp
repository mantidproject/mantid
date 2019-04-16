// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionSingleDomainPresenter.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidQtWidgets/Common/IFunctionView.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace MantidWidgets {

  using namespace Mantid::API;
  using namespace Mantid::Kernel;


FunctionSingleDomainPresenter::FunctionSingleDomainPresenter(IFunctionView *view)
  : m_view(view), m_model(make_unique<SingleDomainFunctionModel>())
{

}

void FunctionSingleDomainPresenter::setFunction(IFunction_sptr fun)
{
  m_model->setFunction(fun);
  m_view->setFunction(fun);
}

IFunction_sptr FunctionSingleDomainPresenter::getFitFunction() const
{
  return m_model->getFitFunction();
}

void FunctionSingleDomainPresenter::setFunctionStr(const std::string & funStr)
{
  m_model->setFunctionStr(funStr);
  m_view->setFunction(m_model->getFitFunction());
}

void FunctionSingleDomainPresenter::clear()
{
  m_model->clear();
  m_view->clear();
}

} // namespace API
} // namespace MantidQt
