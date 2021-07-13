// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvFitModel.h"
#include "IndirectFitAnalysisTab.h"
#include "ParameterEstimation.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IndirectFitTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport IndirectDataAnalysisConvFitTab : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisConvFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "ConvFit"; }

  bool hasResolution() const override { return true; }

protected:
  void setRunIsRunning(bool running) override;
  void setRunEnabled(bool enable) override;

private:
  void setupFitTab() override;
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;
  EstimationDataSelector getEstimationDataSelector() const override;

  std::string getFitTypeString() const;

  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;
  ConvFitModel *m_convFittingModel;
  // ShortHand Naming for fit functions
  std::unordered_map<std::string, std::string> m_fitStrings;

protected slots:
  void runClicked();
  void fitFunctionChanged();
  void setModelResolution(const std::string &resolutionName);
  void setModelResolution(const std::string &resolutionName, WorkspaceID workspaceID);
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
