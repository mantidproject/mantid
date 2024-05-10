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
  IFunction_sptr getFullFunction() const override;
  IFunction_sptr getFitFunction() const override;
  bool hasFunction() const override;
  void addFunction(std::string const &prefix, std::string const &funStr) override;
  void removeFunction(std::string const &functionIndex) override;
  void setParameter(std::string const &parameterName, double value) override;
  void setAttribute(std::string const &attrName, const IFunction::Attribute &val);
  void setParameterError(std::string const &parameterName, double value) override;
  double getParameter(std::string const &parameterName) const override;
  double getParameterError(std::string const &parameterName) const override;
  IFunction::Attribute getAttribute(std::string const &attrName) const;
  std::string getParameterDescription(std::string const &parameterName) const override;
  bool isParameterFixed(std::string const &parameterName) const;
  std::string getParameterTie(std::string const &parameterName) const;
  void setParameterFixed(std::string const &parameterName, bool fixed);
  void setParameterTie(std::string const &parameterName, std::string const &tie);
  std::vector<std::string> getParameterNames() const override;
  std::vector<std::string> getAttributeNames() const;
  IFunction_sptr getSingleFunction(int index) const override;
  IFunction_sptr getCurrentFunction() const override;
  void setNumberDomains(int) override;
  void setDatasets(const std::vector<std::string> &datasetNames);
  void setDatasets(const QList<FunctionModelDataset> &datasets) override;
  void addDatasets(const std::vector<std::string> &datasetNames);
  void removeDatasets(QList<int> &indices);
  std::vector<std::string> getDatasetNames() const override;
  std::vector<std::string> getDatasetDomainNames() const override;
  int getNumberDomains() const override;
  int currentDomainIndex() const override;
  void setCurrentDomainIndex(int) override;
  double getLocalParameterValue(std::string const &parameterName, int i) const override;
  bool isLocalParameterFixed(std::string const &parameterName, int i) const override;
  std::string getLocalParameterTie(std::string const &parameterName, int i) const override;
  std::string getLocalParameterConstraint(std::string const &parameterName, int i) const override;
  void setLocalParameterValue(std::string const &parameterName, int i, double value) override;
  void setLocalParameterValue(std::string const &parameterName, int i, double value, double error) override;
  void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) override;
  void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) override;
  void setLocalParameterConstraint(std::string const &parameterName, int i, std::string const &constraint) override;
  void setGlobalParameterValue(std::string const &parameterName, double value) override;
  void changeTie(std::string const &parameterName, std::string const &tie) override;
  void addConstraint(std::string const &functionIndex, std::string const &constraint) override;
  void removeConstraint(std::string const &parameterName) override;
  std::vector<std::string> getGlobalParameters() const override;
  virtual void setGlobal(std::string const &parameterName, bool on) override;
  void setGlobalParameters(const std::vector<std::string> &globals) override;
  bool isGlobal(std::string const &parameterName) const override;
  std::vector<std::string> getLocalParameters() const override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable) override;
  void updateMultiDatasetAttributes(const IFunction &fun);
  void updateParameters(const IFunction &fun) override;
  std::string setBackgroundA0(double value) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void setQValues(const std::vector<double> &qValues) override;

protected:
  size_t m_numberDomains = 0;
  MultiDomainFunction_sptr m_function;

private:
  IFunction_sptr getFitFunctionWithGlobals(std::size_t const &index) const;

  void checkDatasets();
  void checkNumberOfDomains(const QList<FunctionModelDataset> &datasets) const;
  int numberOfDomains(const QList<FunctionModelDataset> &datasets) const;
  [[nodiscard]] bool checkIndex(int const index) const;
  void updateGlobals();
  void setResolutionFromWorkspace(const IFunction_sptr &fun);
  void setResolutionFromWorkspace(const IFunction_sptr &fun, const MatrixWorkspace_sptr &workspace);
  size_t m_currentDomainIndex = 0;
  // The datasets being fitted. A list of workspace names paired to lists of
  // spectra.
  mutable QList<FunctionModelDataset> m_datasets;
  mutable std::vector<std::string> m_globalParameterNames;
};

} // namespace MantidWidgets
} // namespace MantidQt
