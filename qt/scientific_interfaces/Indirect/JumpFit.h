// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_
#define MANTIDQTCUSTOMINTERFACES_JUMPFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "JumpFitModel.h"
#include "ui_JumpFit.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/TextAxis.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport JumpFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  JumpFit(QWidget *parent = nullptr);

  void setupFitTab() override;

protected slots:
  void updatePlotOptions() override;
  void updateModelFitTypeString();
  void plotClicked();
  void runClicked();

protected:
  bool shouldEnablePlotResult() override;

  void setPlotResultEnabled(bool enabled) override;
  void setSaveResultEnabled(bool enabled) override;

  void setRunIsRunning(bool running) override;

private slots:
  void updateParameterFitTypes();

private:
  void addEISFFunctionsToFitTypeComboBox();
  void addWidthFunctionsToFitTypeComboBox();

  void setRunEnabled(bool enabled);
  void setFitSingleSpectrumEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setPlotResultIsPlotting(bool plotting);

  JumpFitModel *m_jumpFittingModel;
  std::unique_ptr<Ui::JumpFit> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
