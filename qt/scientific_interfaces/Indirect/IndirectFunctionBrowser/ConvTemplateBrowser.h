// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvTemplatePresenter.h"
#include "ConvTypes.h"
#include "DllConfig.h"
#include "FunctionTemplateBrowser.h"

#include <QMap>
#include <QWidget>

class QtProperty;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace ConvTypes;
/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INDIRECT_DLL ConvTemplateBrowser : public FunctionTemplateBrowser {
  Q_OBJECT
public:
  explicit ConvTemplateBrowser(QWidget *parent = nullptr);
  void setFunction(const QString &funStr) override;
  IFunction_sptr getGlobalFunction() const override;
  IFunction_sptr getFunction() const override;
  void setNumberOfDatasets(int) override;
  int getCurrentDataset() override;
  int getNumberOfDatasets() const override;
  void setDatasets(const QList<MantidWidgets::FunctionModelDataset> &datasets) override;
  QStringList getGlobalParameters() const override;
  QStringList getLocalParameters() const override;
  void setGlobalParameters(const QStringList &globals) override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable) override;
  void updateParameters(const IFunction &fun) override;
  void setCurrentDataset(int i) override;
  void updateParameterNames(const QMap<int, QString> &parameterNames) override;
  void setErrorsEnabled(bool enabled) override;
  void clear() override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;

  void setBackgroundA0(double value) override;
  void setResolution(std::string const &name, TableDatasetIndex const &index) override;
  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void addDeltaFunction();
  void removeDeltaFunction();
  void addTempCorrection(double value);
  void removeTempCorrection();
  void setQValues(const std::vector<double> &qValues) override;
  void setEnum(size_t subTypeIndex, int fitType);
  void setInt(size_t subTypeIndex, int val);

  void updateTemperatureCorrectionAndDelta(bool tempCorrection, bool deltaFunction);

protected slots:
  void intChanged(QtProperty *) override;
  void boolChanged(QtProperty *) override;
  void enumChanged(QtProperty *) override;
  void globalChanged(QtProperty *, const QString &, bool) override;
  void parameterChanged(QtProperty *) override;
  void parameterButtonClicked(QtProperty *) override;

private:
  void createProperties() override;
  void popupMenu(const QPoint &) override;
  void setParameterPropertyValue(QtProperty *prop, double value, double error);
  void setGlobalParametersQuiet(const QStringList &globals);
  void createFunctionParameterProperties();
  void createLorentzianFunctionProperties();
  void createDeltaFunctionProperties();
  void createTempCorrectionProperties();
  void setSubType(size_t subTypeIndex, int typeIndex);
  void setParameterValueQuiet(ParamID id, double value, double error);

  std::vector<std::unique_ptr<TemplateSubType>> m_templateSubTypes;
  // Map fit type to a list of function parameters (QtProperties for those
  // parameters)
  std::vector<QMap<int, QList<QtProperty *>>> m_subTypeParameters;
  std::vector<QList<QtProperty *>> m_currentSubTypeParameters;
  std::vector<QtProperty *> m_subTypeProperties;

  QtProperty *m_deltaFunctionOn;
  QtProperty *m_deltaFunctionHeight;
  QtProperty *m_deltaFunctionCenter;

  QtProperty *m_tempCorrectionOn;
  QtProperty *m_temperature;

  QMap<QtProperty *, ParamID> m_parameterMap;
  QMap<ParamID, QtProperty *> m_parameterReverseMap;
  QMap<QtProperty *, QString> m_actualParameterNames;
  QMap<QtProperty *, std::string> m_parameterDescriptions;

private:
  ConvTemplatePresenter m_presenter;
  bool m_emitParameterValueChange = true;
  bool m_emitBoolChange = true;
  bool m_emitEnumChange = true;
  bool m_emitIntChange = true;
  friend class ConvTemplatePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
