// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONMODEL_H_
#define MANTIDWIDGETS_FUNCTIONMODEL_H_

#include "DllOption.h"

#include "IFunctionModel.h"

#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

class EXPORT_OPT_MANTIDQT_COMMON FunctionModel
    : public IFunctionModel {
public:
  void setFunction(IFunction_sptr) override;
  IFunction_sptr getFitFunction() const override;
  bool hasFunction() const override;
  void addFunction(const QString &prefix, const QString &funStr) override;
  void removeFunction(const QString &functionIndex) override;
  void setParameter(const QString &paramName, double value) override;
  void setParameterError(const QString &paramName, double value) override;
  double getParameter(const QString &paramName) const override;
  double getParameterError(const QString &paramName) const override;
  QString getParameterDescription(const QString &paramName) const override;
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
  QString getLocalParameterConstraint(const QString &parName, int i) const;
  void setLocalParameterValue(const QString &parName, int i, double value);
  void setLocalParameterValue(const QString &parName, int i, double value,
                              double error);
  void setLocalParameterFixed(const QString &parName, int i, bool fixed);
  void setLocalParameterTie(const QString &parName, int i, const QString &tie);
  void setLocalParameterConstraint(const QString &parName, int i,
                                   const QString &constraint);
  void changeTie(const QString &parName, const QString &tie) override;
  void addConstraint(const QString &functionIndex,
                     const QString &constraint) override;
  void removeConstraint(const QString &paramName) override;
  QStringList getGlobalParameters() const override;
  void setGlobalParameters(const QStringList &globals) override;
  bool isGlobal(const QString &parName) const override;
  QStringList getLocalParameters() const override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateParameters(const IFunction &fun) override;
  QString setBackgroundA0(double value) override;

private:
  void checkIndex(int) const;
  void updateGlobals();
  MultiDomainFunction_sptr m_function;
  size_t m_currentDomainIndex = 0;
  size_t m_numberDomains = 0;
  mutable QStringList m_datasetNames;
  mutable QStringList m_globalParameterNames;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDWIDGETS_FUNCTIONMODEL_H_
