// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ITemplatePresenter.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/IFunctionModel.h"

#include <QList>
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class FunctionTemplateBrowser;

class MANTIDQT_INELASTIC_DLL FunctionTemplatePresenter : public ITemplatePresenter {
public:
  using ITemplatePresenter::updateMultiDatasetParameters;

  FunctionTemplatePresenter(FunctionTemplateBrowser *view, std::unique_ptr<MantidWidgets::IFunctionModel> model);

  FunctionTemplateBrowser *browser() override { return m_view; }

  virtual void init() override;
  virtual void
  updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings) override;

  void setNumberOfDatasets(int) override;
  int getNumberOfDatasets() const override;
  int getCurrentDataset() override;

  Mantid::API::IFunction_sptr getGlobalFunction() const override;
  Mantid::API::IFunction_sptr getFunction() const override;

  std::vector<std::string> getGlobalParameters() const override;
  std::vector<std::string> getLocalParameters() const override;

  void setDatasets(const QList<MantidWidgets::FunctionModelDataset> &datasets) override;

  void setErrorsEnabled(bool enabled) override;

  void handleEditLocalParameter(std::string const &parameterName) override;
  void handleParameterValueChanged(std::string const &parameterName, double value) override;

  virtual void setFitType(std::string const &name) override;

  virtual void updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &table) override;

  /// Used by IqtTemplatePresenter
  virtual void setNumberOfExponentials(int nExponentials) override;
  virtual void setStretchExponential(bool on) override;
  virtual void setBackground(std::string const &name) override;
  virtual void tieIntensities(bool on) override;
  virtual bool canTieIntensities() const override;

  /// Used by ConvTemplatePresenter
  virtual void setSubType(std::size_t subTypeIndex, int typeIndex) override;
  virtual void setDeltaFunction(bool on) override;
  virtual void setTempCorrection(bool on) override;
  virtual void setBackgroundA0(double value) override;
  virtual void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  virtual void setQValues(const std::vector<double> &qValues) override;

protected:
  QStringList getDatasetNames() const;
  QStringList getDatasetDomainNames() const;

  void setLocalParameterValue(std::string const &parameterName, int i, double value);
  void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie);
  void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed);
  double getLocalParameterValue(std::string const &parameterName, int i) const;
  bool isLocalParameterFixed(std::string const &parameterName, int i) const;
  std::string getLocalParameterTie(std::string const &parameterName, int i) const;
  std::string getLocalParameterConstraint(std::string const &parameterName, int i) const;

  FunctionTemplateBrowser *m_view;
  std::unique_ptr<MantidWidgets::IFunctionModel> m_model;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt