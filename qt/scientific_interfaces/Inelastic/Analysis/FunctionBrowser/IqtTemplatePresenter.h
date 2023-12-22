// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/ParameterEstimation.h"
#include "DllConfig.h"
#include "IqtFunctionModel.h"

#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace IDA {

class IqtTemplateBrowser;

/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INELASTIC_DLL IqtTemplatePresenter : public QObject {
  Q_OBJECT
public:
  explicit IqtTemplatePresenter(IqtTemplateBrowser *view, std::unique_ptr<IqtFunctionModel> functionModel);
  void setNumberOfExponentials(int);
  void setStretchExponential(bool);
  void setBackground(std::string const &name);
  void setNumberOfDatasets(int);
  int getNumberOfDatasets() const;
  void setFunction(std::string const &funStr);
  IFunction_sptr getGlobalFunction() const;
  IFunction_sptr getFunction() const;
  std::vector<std::string> getGlobalParameters() const;
  std::vector<std::string> getLocalParameters() const;
  void setGlobalParameters(std::vector<std::string> const &globals);
  void setGlobal(std::string const &parameterName, bool on);
  void updateMultiDatasetParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable);
  void updateParameters(const IFunction &fun);
  void setCurrentDataset(int i);
  int getCurrentDataset();
  void setDatasets(const QList<FunctionModelDataset> &datasets);
  void setViewParameterDescriptions();
  void setErrorsEnabled(bool enabled);
  void tieIntensities(bool on);
  bool canTieIntensities() const;
  EstimationDataSelector getEstimationDataSelector() const;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters();
  void setBackgroundA0(double value);

signals:
  void functionStructureChanged();

private slots:
  void editLocalParameter(std::string const &parameterName);
  void editLocalParameterFinish(int result);
  void viewChangedParameterValue(std::string const &parameterName, double value);

private:
  void updateViewParameters();
  QStringList getDatasetNames() const;
  QStringList getDatasetDomainNames() const;
  double getLocalParameterValue(std::string const &parameterName, int i) const;
  bool isLocalParameterFixed(std::string const &parameterName, int i) const;
  std::string getLocalParameterTie(std::string const &parameterName, int i) const;
  std::string getLocalParameterConstraint(std::string const &parameterName, int i) const;
  void setLocalParameterValue(std::string const &parameterName, int i, double value);
  void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed);
  void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie);
  void updateViewParameterNames();
  void updateView();
  IqtTemplateBrowser *m_view;
  std::unique_ptr<IqtFunctionModel> m_model;
  EditLocalParameterDialog *m_editLocalParameterDialog;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
