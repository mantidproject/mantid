// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "IFunctionModel.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

class EXPORT_OPT_MANTIDQT_COMMON FunctionModel : public IFunctionModel {
public:
  void setFunction(IFunction_sptr) override;
  IFunction_sptr getFitFunction() const override;
  bool hasFunction() const override;
  void addFunction(const QString &prefix, const QString &funStr) override;
  void removeFunction(const QString &functionIndex) override;
  void setParameter(const QString &paramName, double value) override;
  void setAttribute(const QString &attrName, const IFunction::Attribute &val);
  void setParameterError(const QString &paramName, double value) override;
  double getParameter(const QString &paramName) const override;
  double getParameterError(const QString &paramName) const override;
  IFunction::Attribute getAttribute(const QString &attrName) const;
  QString getParameterDescription(const QString &paramName) const override;
  bool isParameterFixed(const QString &parName) const;
  QString getParameterTie(const QString &parName) const;
  void setParameterFixed(const QString &parName, bool fixed);
  void setParameterTie(const QString &parName, const QString &tie);
  QStringList getParameterNames() const override;
  QStringList getAttributeNames() const;
  IFunction_sptr getSingleFunction(int index) const override;
  IFunction_sptr getCurrentFunction() const override;
  void setNumberDomains(int) override;
  void setDatasets(const QStringList &datasetNames);
  void setDatasets(const QList<FunctionModelDataset> &datasets) override;
  void addDatasets(const QStringList &datasetNames);
  void removeDatasets(QList<int> &indices);
  QStringList getDatasetNames() const override;
  QStringList getDatasetDomainNames() const override;
  int getNumberDomains() const override;
  int currentDomainIndex() const override;
  void setCurrentDomainIndex(int) override;
  double getLocalParameterValue(const QString &parName, int i) const override;
  bool isLocalParameterFixed(const QString &parName, int i) const override;
  QString getLocalParameterTie(const QString &parName, int i) const override;
  QString getLocalParameterConstraint(const QString &parName, int i) const override;
  void setLocalParameterValue(const QString &parName, int i, double value) override;
  void setLocalParameterValue(const QString &parName, int i, double value, double error) override;
  void setLocalParameterFixed(const QString &parName, int i, bool fixed) override;
  void setLocalParameterTie(const QString &parName, int i, const QString &tie) override;
  void setLocalParameterConstraint(const QString &parName, int i, const QString &constraint) override;
  void setGlobalParameterValue(const QString &paramName, double value) override;
  void changeTie(const QString &parName, const QString &tie) override;
  void addConstraint(const QString &functionIndex, const QString &constraint) override;
  void removeConstraint(const QString &paramName) override;
  QStringList getGlobalParameters() const override;
  void setGlobalParameters(const QStringList &globals) override;
  bool isGlobal(const QString &parName) const override;
  QStringList getLocalParameters() const override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetAttributes(const IFunction &fun);
  void updateParameters(const IFunction &fun) override;
  QString setBackgroundA0(double value) override;

protected:
  size_t m_numberDomains = 0;
  MultiDomainFunction_sptr m_function;

private:
  IFunction_sptr getFitFunctionWithGlobals(std::size_t const &index) const;

  void checkDatasets();
  void checkNumberOfDomains(const QList<FunctionModelDataset> &datasets) const;
  int numberOfDomains(const QList<FunctionModelDataset> &datasets) const;
  void checkIndex(int) const;
  void updateGlobals();
  void setResolutionFromWorkspace(IFunction_sptr fun);
  void setResolutionFromWorkspace(IFunction_sptr fun, const MatrixWorkspace_sptr workspace);
  size_t m_currentDomainIndex = 0;
  // The datasets being fitted. A list of workspace names paired to lists of
  // spectra.
  mutable QList<FunctionModelDataset> m_datasets;
  mutable QStringList m_globalParameterNames;
};

} // namespace MantidWidgets
} // namespace MantidQt
