// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONMODEL_H_
#define MANTIDWIDGETS_FUNCTIONMODEL_H_

#include "DllOption.h"

#include "MantidAPI/IFunction_fwd.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {

  using namespace Mantid::API;

  class EXPORT_OPT_MANTIDQT_COMMON IFunctionModel {
  public:
    virtual ~IFunctionModel() {}
    virtual void setFunction(IFunction_sptr fun) = 0;
    virtual IFunction_sptr getFitFunction() const = 0;
    void setFunctionStr(const std::string &funStr);
  private:
  };

  class EXPORT_OPT_MANTIDQT_COMMON SingleDomainFunctionModel: public IFunctionModel {
  public:
    void setFunction(IFunction_sptr fun) override;
    IFunction_sptr getFitFunction() const override;
  private:
    CompositeFunction_sptr m_function;
  };

  class EXPORT_OPT_MANTIDQT_COMMON MultiDomainFunctionModel : public IFunctionModel {
  public:
    void setFunction(IFunction_sptr) override;
    IFunction_sptr getFitFunction() const override;
    IFunction_sptr getSingleFunction(int index) const;
    IFunction_sptr getCurrentFunction() const;
    void setNumberDomains(int);
    int getNumberDomains() const;
    int currentDomainIndex() const;
    void setCurrentDomainIndex(int);
  private:
    void checkIndex(int) const;
    MultiDomainFunction_sptr m_function;
    size_t m_currentDomainIndex = 0;
    size_t m_numberDomains = 1;
  };

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMODEL_H_
