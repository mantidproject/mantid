// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FqFitModel.h"
#include "IndirectFitAnalysisTab.h"
#include "ui_IndirectFitTab.h"

#include "IFQFitObserver.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/TextAxis.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class IDAFunctionParameterEstimation;

class DLLExport IndirectDataAnalysisFqFitTab : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisFqFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "FQFit"; }

  bool hasResolution() const override { return false; }

private:
  void setupFitTab() override;
  EstimationDataSelector getEstimationDataSelector() const override;
  std::string getFitTypeString() const;
  IDAFunctionParameterEstimation createParameterEstimation() const;

  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;
  FqFitModel *m_FqFittingModel;

protected:
  void setRunIsRunning(bool running) override;
  void setRunEnabled(bool enable) override;

protected slots:
  void runClicked();
  void updateModelFitTypeString();
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
