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
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

  using namespace Mantid::API;

  class EXPORT_OPT_MANTIDQT_COMMON IFunctionModel {
  public:
    virtual ~IFunctionModel() {}
    void setFunctionString(const QString &funStr);
    QString getFunctionString() const;
    QString getFitFunctionString() const;
    void clear();
    virtual void setFunction(IFunction_sptr fun) = 0;
    virtual IFunction_sptr getFitFunction() const = 0;
    virtual bool hasFunction() const = 0;
    virtual void addFunction(const QString &prefix, const QString &funStr) = 0;
    virtual void setParameter(const QString &paramName, double value) = 0;
    virtual void setParamError(const QString &paramName, double value) = 0;
    virtual double getParameter(const QString &paramName) const = 0;
    virtual double getParamError(const QString &paramName) const = 0;
    virtual QStringList getParameterNames() const = 0;
    virtual IFunction_sptr getSingleFunction(int index) const = 0;
    virtual IFunction_sptr getCurrentFunction() const = 0;
    virtual void setNumberDomains(int) = 0;
    virtual void setDatasetNames(const QStringList &names) = 0;
    virtual QStringList getDatasetNames() const = 0;
    virtual int getNumberDomains() const = 0;
    virtual int currentDomainIndex() const = 0;
    virtual void setCurrentDomainIndex(int) = 0;
  };

  class EXPORT_OPT_MANTIDQT_COMMON MultiDomainFunctionModel : public IFunctionModel {
  public:
    void setFunction(IFunction_sptr) override;
    IFunction_sptr getFitFunction() const override;
    bool hasFunction() const override;
    void addFunction(const QString &prefix, const QString &funStr) override;
    void setParameter(const QString &paramName, double value) override;
    void setParamError(const QString &paramName, double value) override;
    double getParameter(const QString &paramName) const override;
    double getParamError(const QString &paramName) const override;
    QStringList getParameterNames() const override;
    IFunction_sptr getSingleFunction(int index) const override;
    IFunction_sptr getCurrentFunction() const override;
    void setNumberDomains(int) override;
    void setDatasetNames(const QStringList &names) override;
    QStringList getDatasetNames() const override;
    int getNumberDomains() const override;
    int currentDomainIndex() const override;
    void setCurrentDomainIndex(int) override;
    double getLocalParameterValue(const QString &parName, int i) const;
    bool isLocalParameterFixed(const QString &parName, int i) const;
    QString getLocalParameterTie(const QString &parName, int i) const;
    void setLocalParameterValue(const QString &parName, int i, double value);
    void setLocalParameterFixed(const QString &parName, int i, bool fixed);
    void setLocalParameterTie(const QString &parName, int i, QString tie);
  private:
    void checkIndex(int) const;
    MultiDomainFunction_sptr m_function;
    size_t m_currentDomainIndex = 0;
    size_t m_numberDomains = 1;
    mutable QStringList m_datasetNames;
  };

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMODEL_H_
