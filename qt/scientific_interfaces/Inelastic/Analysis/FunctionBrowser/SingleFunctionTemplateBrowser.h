// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/FunctionTemplateBrowser.h"
#include "DllConfig.h"
#include "SingleFunctionTemplatePresenter.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IDAFunctionParameterEstimation;
/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INELASTIC_DLL SingleFunctionTemplateBrowser : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  explicit SingleFunctionTemplateBrowser(std::unique_ptr<SingleFunctionTemplateModel> functionModel,
                                         QWidget *parent = nullptr);
  virtual ~SingleFunctionTemplateBrowser() = default;
  void setFunction(std::string const &funStr) override;
  IFunction_sptr getGlobalFunction() const override;
  IFunction_sptr getFunction() const override;
  void setNumberOfDatasets(int) override;
  int getNumberOfDatasets() const override;
  void setDatasets(const QList<MantidWidgets::FunctionModelDataset> &datasets) override;
  std::vector<std::string> getGlobalParameters() const override;
  std::vector<std::string> getLocalParameters() const override;
  void setGlobalParameters(std::vector<std::string> const &globals) override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable) override;
  void updateParameters(const IFunction &fun) override;
  void setCurrentDataset(int i) override;
  void updateParameterNames(const QMap<int, std::string> &parameterNames) override;
  void updateParameterDescriptions(const QMap<int, std::string> &parameterNames);
  void setErrorsEnabled(bool enabled) override;
  void clear() override;
  EstimationDataSelector getEstimationDataSelector() const override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;
  void setBackgroundA0(double) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &) override;
  void setQValues(const std::vector<double> &) override;
  int getCurrentDataset() override;
  void addParameter(std::string const &parameterName, std::string const &parameterDescription);
  void setParameterValue(std::string const &parameterName, double parameterValue, double parameterError);
  void setParameterValueQuietly(std::string const &parameterName, double parameterValue, double parameterError);
  void setDataType(std::vector<std::string> const &allowedFunctionsList);
  void setEnumValue(int enumIndex);
  void updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings);

protected slots:
  void enumChanged(QtProperty *) override;
  void globalChanged(QtProperty *, const QString &, bool) override;
  void parameterChanged(QtProperty *) override;
  void parameterButtonClicked(QtProperty *) override;

private:
  void createProperties() override;
  void popupMenu(const QPoint &) override;
  void setParameterPropertyValue(QtProperty *prop, double value, double error);
  void setGlobalParametersQuiet(std::vector<std::string> const &globals);

  QtProperty *m_fitType;
  QMap<std::string, QtProperty *> m_parameterMap;
  QMap<QtProperty *, std::string> m_parameterNames;

private:
  SingleFunctionTemplatePresenter m_presenter;
  bool m_emitParameterValueChange = true;
  bool m_emitBoolChange = true;
  bool m_emitEnumChange = true;
  friend class SingleFunctionTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
