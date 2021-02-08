// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "FunctionModelDataset.h"
#include "MantidAPI/IFunction_fwd.h"

#include <QList>
#include <QPair>
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
  int getNumberLocalFunctions() const;
  virtual void setFunction(IFunction_sptr fun) = 0;
  virtual IFunction_sptr getFitFunction() const = 0;
  virtual bool hasFunction() const = 0;
  virtual void addFunction(const QString &prefix, const QString &funStr) = 0;
  virtual void removeFunction(const QString &functionIndex) = 0;
  virtual void setParameter(const QString &paramName, double value) = 0;
  virtual void setParameterError(const QString &paramName, double value) = 0;
  virtual double getParameter(const QString &paramName) const = 0;
  virtual double getParameterError(const QString &paramName) const = 0;
  virtual QString getParameterDescription(const QString &paramName) const = 0;
  virtual QStringList getParameterNames() const = 0;
  virtual IFunction_sptr getSingleFunction(int index) const = 0;
  virtual IFunction_sptr getCurrentFunction() const = 0;
  virtual void setNumberDomains(int) = 0;
  virtual void setDatasets(const QList<FunctionModelDataset> &datasets) = 0;
  virtual QStringList getDatasetNames() const = 0;
  virtual QStringList getDatasetDomainNames() const = 0;
  virtual int getNumberDomains() const = 0;
  virtual int currentDomainIndex() const = 0;
  virtual void setCurrentDomainIndex(int) = 0;
  virtual void changeTie(const QString &paramName, const QString &tie) = 0;
  virtual void addConstraint(const QString &functionIndex,
                             const QString &constraint) = 0;
  virtual void removeConstraint(const QString &paramName) = 0;
  virtual QStringList getGlobalParameters() const = 0;
  virtual void setGlobalParameters(const QStringList &globals) = 0;
  virtual bool isGlobal(const QString &parName) const = 0;
  virtual QStringList getLocalParameters() const = 0;
  virtual void updateMultiDatasetParameters(const IFunction &fun) = 0;
  virtual void updateParameters(const IFunction &fun) = 0;
  virtual double getLocalParameterValue(const QString &parName,
                                        int i) const = 0;
  virtual bool isLocalParameterFixed(const QString &parName, int i) const = 0;
  virtual QString getLocalParameterTie(const QString &parName, int i) const = 0;
  virtual QString getLocalParameterConstraint(const QString &parName,
                                              int i) const = 0;
  virtual void setLocalParameterValue(const QString &parName, int i,
                                      double value) = 0;
  virtual void setLocalParameterValue(const QString &parName, int i,
                                      double value, double error) = 0;
  virtual void setLocalParameterFixed(const QString &parName, int i,
                                      bool fixed) = 0;
  virtual void setLocalParameterTie(const QString &parName, int i,
                                    const QString &tie) = 0;
  virtual void setLocalParameterConstraint(const QString &parName, int i,
                                           const QString &constraint) = 0;
  virtual void setGlobalParameterValue(const QString &paramName,
                                       double value) = 0;
  virtual QString setBackgroundA0(double value) = 0;

protected:
  static void copyParametersAndErrors(const IFunction &funFrom,
                                      IFunction &funTo);
  void copyParametersAndErrorsToAllLocalFunctions(const IFunction &fun);
};

} // namespace MantidWidgets
} // namespace MantidQt
