// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvFunctionModel.h"
#include "DllConfig.h"
#include "FunctionTemplatePresenter.h"

#include <QMap>

class QtProperty;

namespace MantidQt {
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace IDA {

class ConvTemplateBrowser;

/**
 * Class FunctionTemplateBrowser implements QtPropertyBrowser to display
 * and set properties that can be used to generate a fit function.
 *
 */
class MANTIDQT_INELASTIC_DLL ConvTemplatePresenter : public FunctionTemplatePresenter {
public:
  explicit ConvTemplatePresenter(ConvTemplateBrowser *view, std::unique_ptr<ConvFunctionModel> model);
  FunctionTemplateBrowser *browser() override { return reinterpret_cast<FunctionTemplateBrowser *>(m_view); }
  ConvTemplateBrowser *view() const;
  ConvFunctionModel *model() const;

  void setSubType(size_t subTypeIndex, int typeIndex) override;
  void setDeltaFunction(bool) override;
  void setTempCorrection(bool) override;

  void setNumberOfDatasets(int) override;
  int getNumberOfDatasets() const override;
  int getCurrentDataset() override;

  void setFunction(std::string const &funStr) override;
  IFunction_sptr getGlobalFunction() const override;
  IFunction_sptr getFunction() const override;

  std::vector<std::string> getGlobalParameters() const override;
  std::vector<std::string> getLocalParameters() const override;
  void setGlobalParameters(std::vector<std::string> const &globals) override;
  void setGlobal(std::string const &parameterName, bool on) override;

  void updateMultiDatasetParameters(const Mantid::API::IFunction &fun) override;
  void updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &table) override;
  void updateParameters(const IFunction &fun) override;

  void setCurrentDataset(int i) override;
  void setDatasets(const QList<FunctionModelDataset> &datasets) override;

  void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void setBackgroundA0(double value) override;
  void setQValues(const std::vector<double> &qValues) override;

  EstimationDataSelector getEstimationDataSelector() const override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;

  void setErrorsEnabled(bool enabled) override;

  void handleEditLocalParameter(std::string const &parameterName) override;
  void handleParameterValueChanged(std::string const &parameterName, double value) override;
  void handleEditLocalParameterFinished(std::string const &parameterName, QList<double> const &values,
                                        QList<bool> const &fixes, QStringList const &ties,
                                        QStringList const &constraints) override;

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
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
