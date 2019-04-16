// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRESENTER_H_
#define MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRESENTER_H__

#include "DllOption.h"

#include "MantidAPI/IFunction_fwd.h"

#include <QString>

namespace MantidQt {
  namespace MantidWidgets {

    using namespace Mantid::API;

    class EXPORT_OPT_MANTIDQT_COMMON FunctionMultiDomainPresenter {
    public:
      FunctionMultiDomainPresenter();
      void setFunction(IFunction_sptr fun);
      IFunction_sptr getFitFunction() const;
      void setFunctionStr(const std::string &funStr);
      void clear();
    private:
    };
  } // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRESENTER_H_
