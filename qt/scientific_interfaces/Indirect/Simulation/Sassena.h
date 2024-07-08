// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "SimulationTab.h"
#include "ui_Sassena.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL Sassena : public SimulationTab {
  Q_OBJECT

public:
  Sassena(QWidget *parent = nullptr);

  void setup() override;
  bool validate() override;
  void run() override;

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

private slots:
  /// Handle completion of the algorithm batch
  void handleAlgorithmFinish(bool error);
  void runClicked();
  void saveClicked();

private:
  void setRunIsRunning(bool running);
  void setButtonsEnabled(bool enabled);
  void setRunEnabled(bool enabled);
  void setSaveEnabled(bool enabled);

  /// The ui form
  Ui::Sassena m_uiForm;
  /// Name of the output workspace group
  QString m_outWsName;
};
} // namespace CustomInterfaces
} // namespace MantidQt
