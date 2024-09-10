// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/IFunction_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"

#include <QObject>
#include <memory>
#include <optional>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class FunctionModel;
class IFunctionView;
class EditLocalParameterDialog;

using namespace Mantid::API;

class EXPORT_OPT_MANTIDQT_COMMON FunctionMultiDomainPresenter : public QObject {
  Q_OBJECT
public:
  FunctionMultiDomainPresenter(IFunctionView *view);
  void clear();
  void setFunction(IFunction_sptr fun);
  void setFunctionString(std::string const &funStr);
  std::string getFunctionString() const;
  IFunction_sptr getFunction() const;
  IFunction_sptr getFunctionByIndex(std::string const &index);
  IFunction_sptr getFitFunction() const;
  std::string getFitFunctionString() const;
  bool hasFunction() const;
  void setParameter(std::string const &parameterName, double value);
  void setParameterError(std::string const &parameterName, double value);
  double getParameter(std::string const &parameterName);
  bool isParameterFixed(std::string const &parameterName) const;
  std::string getParameterTie(std::string const &parameterName) const;
  void updateParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const IFunction &fun);
  void updateMultiDatasetAttributes(const IFunction &fun);
  void clearErrors();
  std::optional<std::string> currentFunctionIndex() const;
  void setNumberOfDatasets(int);
  void setDatasets(const std::vector<std::string> &datasetNames);
  void setDatasets(const QList<FunctionModelDataset> &datasets);
  void addDatasets(const std::vector<std::string> &datasetNames);
  std::vector<std::string> getDatasetNames() const;
  std::vector<std::string> getDatasetDomainNames() const;
  int getNumberOfDatasets() const;
  int getCurrentDataset() const;
  void setCurrentDataset(int);
  void removeDatasets(QList<int> indices);
  double getLocalParameterValue(std::string const &parameterName, int i) const;
  bool isLocalParameterFixed(std::string const &parameterName, int i) const;
  std::string getLocalParameterTie(std::string const &parameterName, int i) const;
  std::string getLocalParameterConstraint(std::string const &parameterName, int i) const;
  void setLocalParameterValue(std::string const &parameterName, int i, double value);
  void setLocalParameterValue(std::string const &parameterName, int i, double value, double error);
  void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed);
  void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie);
  void setLocalParameterConstraint(std::string const &parameterName, int i, std::string const &constraint);
  std::vector<std::string> getGlobalParameters() const;
  void setGlobalParameters(std::vector<std::string> const &globals);
  std::vector<std::string> getLocalParameters() const;
  void setBackgroundA0(double value);

  void setColumnSizes(int s0, int s1, int s2);
  void setStretchLastColumn(bool stretch);
  void setErrorsEnabled(bool enabled);
  void hideGlobals();
  void showGlobals();
signals:
  void functionStructureChanged();
  void parameterChanged(std::string const &funcIndex, std::string const &parameterName);
  void attributeChanged(std::string const &attributeName);
private slots:
  void viewChangedParameter(std::string const &parameterName);
  void viewChangedAttribute(std::string const &attrName);
  void viewPastedFunction(std::string const &funStr);
  void viewAddedFunction(std::string const &funStr);
  void viewRemovedFunction(std::string const &functionIndex);
  void viewChangedTie(std::string const &parameterName, std::string const &tie);
  void viewAddedConstraint(std::string const &functionIndex, std::string const &constraint);
  void viewRemovedConstraint(std::string const &parameterName);
  void viewRequestedCopyToClipboard();
  void viewChangedGlobals(const std::vector<std::string> &globalParameters);
  void editLocalParameter(std::string const &parameterName);
  void editLocalParameterFinish(int result);
  void viewRequestedFunctionHelp();

private:
  void updateViewFromModel();
  void updateViewAttributesFromModel();

private:
  IFunctionView *m_view;
  std::unique_ptr<FunctionModel> m_model;
  EditLocalParameterDialog *m_editLocalParameterDialog;

public:
  IFunctionView *view() const { return m_view; }
};

} // namespace MantidWidgets
} // namespace MantidQt
