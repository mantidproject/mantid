// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_

#include "IndirectSimulationTab.h"
#include "ui_IndirectSassena.h"

namespace MantidQt {
namespace CustomInterfaces {
class DLLExport IndirectSassena : public IndirectSimulationTab {
  Q_OBJECT

public:
  IndirectSassena(QWidget *parent = nullptr);

  void setup() override;
  bool validate() override;
  void run() override;

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

private slots:
  /// Handle completion of the algorithm batch
  void handleAlgorithmFinish(bool error);
  void runClicked();
  void plotClicked();
  void saveClicked();

private:
  void setRunIsRunning(bool running);
  void setPlotIsPlotting(bool plotting);
  void setButtonsEnabled(bool enabled);
  void setRunEnabled(bool enabled);
  void setPlotEnabled(bool enabled);
  void setSaveEnabled(bool enabled);

  /// The ui form
  Ui::IndirectSassena m_uiForm;
  /// Name of the output workspace group
  QString m_outWsName;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECTSASSENA_H_
