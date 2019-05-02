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
    virtual void removeFunction(const QString &functionIndex) = 0;
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
    virtual void changeTie(const QString & paramName, const QString & tie) = 0;
    virtual void addConstraint(const QString &functionIndex, const QString &constraint) = 0;
    virtual void removeConstraint(const QString &paramName) = 0;
    virtual QStringList getGlobalParameters() const = 0;
    virtual void setGlobalParameters(const QStringList &globals) = 0;
    virtual QStringList getLocalParameters() const = 0;
  };

  class EXPORT_OPT_MANTIDQT_COMMON MultiDomainFunctionModel : public IFunctionModel {
  public:
    void setFunction(IFunction_sptr) override;
    IFunction_sptr getFitFunction() const override;
    bool hasFunction() const override;
    void addFunction(const QString &prefix, const QString &funStr) override;
    void removeFunction(const QString &functionIndex) override;
    void setParameter(const QString &paramName, double value) override;
    void setParamError(const QString &paramName, double value) override;
    double getParameter(const QString &paramName) const override;
    double getParamError(const QString &paramName) const override;
    bool isParameterFixed(const QString &parName) const;
    QString getParameterTie(const QString &parName) const;
    void setParameterFixed(const QString &parName, bool fixed);
    void setParameterTie(const QString &parName, QString tie);
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
    void setLocalParameterValue(const QString &parName, int i, double value, double error);
    void setLocalParameterFixed(const QString &parName, int i, bool fixed);
    void setLocalParameterTie(const QString &parName, int i, QString tie);
    void changeTie(const QString & parName, const QString & tie);
    void addConstraint(const QString &functionIndex, const QString &constraint) override;
    void removeConstraint(const QString &paramName) override;
    QStringList getGlobalParameters() const override;
    void setGlobalParameters(const QStringList &globals) override;
    QStringList getLocalParameters() const override;
  private:
    void checkIndex(int) const;
    MultiDomainFunction_sptr m_function;
    size_t m_currentDomainIndex = 0;
    size_t m_numberDomains = 0;
    mutable QStringList m_datasetNames;
    QStringList m_globalParameterNames;
  };

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMODEL_H_
