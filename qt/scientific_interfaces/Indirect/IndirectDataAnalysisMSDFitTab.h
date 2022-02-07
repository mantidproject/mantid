// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFitAnalysisTab.h"
#include "MSDFitModel.h"
#include "ui_IndirectFitTab.h"

#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class IDAFunctionParameterEstimation;

class DLLExport IndirectDataAnalysisMSDFitTab : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisMSDFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "MSDFit"; }

  bool hasResolution() const override { return false; }

protected:
  void setRunIsRunning(bool running) override;
  void setRunEnabled(bool enable) override;

private:
  void setupFitTab() override;
  EstimationDataSelector getEstimationDataSelector() const override;
  std::string getFitTypeString() const;
  IDAFunctionParameterEstimation createParameterEstimation() const;
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;

  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;
  MSDFitModel *m_msdFittingModel;

protected slots:
  void runClicked();
  void fitFunctionChanged();
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
