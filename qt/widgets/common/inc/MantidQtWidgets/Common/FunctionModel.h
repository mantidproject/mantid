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
    void setFunctionString(const QString &funStr);
    QString getFunctionString() const;
    void clear();
    virtual void setFunction(IFunction_sptr fun) = 0;
    virtual IFunction_sptr getFitFunction() const = 0;
    virtual void setParameter(const QString &paramName, double value) = 0;
    virtual void setParamError(const QString &paramName, double value) = 0;
    virtual double getParameter(const QString &paramName) const = 0;
    virtual double getParamError(const QString &paramName) const = 0;
    virtual IFunction_sptr getSingleFunction(int index) const = 0;
    virtual IFunction_sptr getCurrentFunction() const = 0;
    virtual void setNumberDomains(int) = 0;
    virtual int getNumberDomains() const = 0;
    virtual int currentDomainIndex() const = 0;
    virtual void setCurrentDomainIndex(int) = 0;
  };

  class EXPORT_OPT_MANTIDQT_COMMON MultiDomainFunctionModel : public IFunctionModel {
  public:
    void setFunction(IFunction_sptr) override;
    IFunction_sptr getFitFunction() const override;
    void setParameter(const QString &paramName, double value) override;
    void setParamError(const QString &paramName, double value) override;
    double getParameter(const QString &paramName) const override;
    double getParamError(const QString &paramName) const override;
    IFunction_sptr getSingleFunction(int index) const override;
    IFunction_sptr getCurrentFunction() const override;
    void setNumberDomains(int) override;
    int getNumberDomains() const override;
    int currentDomainIndex() const override;
    void setCurrentDomainIndex(int) override;
  private:
    void checkIndex(int) const;
    MultiDomainFunction_sptr m_function;
    size_t m_currentDomainIndex = 0;
    size_t m_numberDomains = 1;
  };

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMODEL_H_
