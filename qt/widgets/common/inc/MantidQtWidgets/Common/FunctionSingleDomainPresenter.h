// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONSINGLEDOMAINPRESENTER_H_
#define MANTIDWIDGETS_FUNCTIONSINGLEDOMAINPRESENTER_H_

#include "DllOption.h"

#include "MantidAPI/IFunction_fwd.h"

#include <memory>

namespace MantidQt {
namespace MantidWidgets {

  class SingleDomainFunctionModel;
  class IFunctionView;

  using namespace Mantid::API;

  class EXPORT_OPT_MANTIDQT_COMMON FunctionSingleDomainPresenter {
  public:
    FunctionSingleDomainPresenter(IFunctionView *view);
    void setFunction(IFunction_sptr fun);
    IFunction_sptr getFitFunction() const;
    void setFunctionStr(const std::string &funStr);
    void clear();
  private:
    IFunctionView *m_view;
    std::unique_ptr<SingleDomainFunctionModel> m_model;
  };
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONSINGLEDOMAINPRESENTER_H_
