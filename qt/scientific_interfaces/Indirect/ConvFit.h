// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ConvFitModel.h"
#include "IndirectFitAnalysisTab.h"
#include "IndirectSpectrumSelectionPresenter.h"
#include "ParameterEstimation.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IndirectFitTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport ConvFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  ConvFit(QWidget *parent = nullptr);

  std::string tabName() const override { return "ConvFit"; }

  bool hasResolution() const override { return true; }

protected slots:
  void setModelResolution(const std::string &resolutionName);
  void setModelResolution(const std::string &resolutionName,
                          TableDatasetIndex index);
  void runClicked();
  void fitFunctionChanged();

protected:
  void setRunIsRunning(bool running) override;
  void setRunEnabled(bool enable) override;

private:
  void setupFitTab() override;
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;
  EstimationDataSelector getEstimationDataSelector() const override;

  std::string fitTypeString() const;

  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;
  // ShortHand Naming for fit functions
  std::unordered_map<std::string, std::string> m_fitStrings;
  ConvFitModel *m_convFittingModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
