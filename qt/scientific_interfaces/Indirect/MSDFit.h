// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "MSDFitModel.h"
#include "ui_MSDFit.h"

#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport MSDFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  MSDFit(QWidget *parent = nullptr);

protected slots:
  void plotClicked();
  void runClicked();
  void updatePlotOptions() override;
  void updateModelFitTypeString();

protected:
	void setRunIsRunning(bool running) override;
	void setFitSingleSpectrumIsFitting(bool fitting) override;
  void setPlotResultEnabled(bool enabled) override;
  void setSaveResultEnabled(bool enabled) override;

private:
  void setupFitTab() override;

	void setPlotResultIsPlotting(bool plotting);
	void setButtonsEnabled(bool enabled);
  void setRunEnabled(bool enabled);
  void setFitSingleSpectrumEnabled(bool enabled);

  MSDFitModel *m_msdFittingModel;
  std::unique_ptr<Ui::MSDFit> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_MSDFIT_H_ */
