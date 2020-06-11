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
class DLLExport MSDFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  MSDFit(QWidget *parent = nullptr);

  std::string tabName() const override { return "MSDFit"; }

  bool hasResolution() const override { return false; }

protected slots:
  void runClicked();
  void fitFunctionChanged();

protected:
  void setRunIsRunning(bool running) override;
  void setRunEnabled(bool enable) override;

private:
  void setupFitTab() override;
  EstimationDataSelector getEstimationDataSelector() const override;
  std::string fitTypeString() const;
  MSDFitModel *m_msdFittingModel;
  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
