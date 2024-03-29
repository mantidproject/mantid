// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "IndirectSimulationTab.h"
#include "ui_IndirectMolDyn.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL IndirectMolDyn : public IndirectSimulationTab {
  Q_OBJECT

public:
  IndirectMolDyn(QWidget *parent = nullptr);

  // Inherited methods from IndirectTab
  void setup() override;
  bool validate() override;
  void run() override;

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

private slots:
  void versionSelected(const QString & /*version*/);
  void runClicked();
  void saveClicked();
  void algorithmComplete(bool error);

private:
  void setRunIsRunning(bool running);
  void setButtonsEnabled(bool enabled);
  void setRunEnabled(bool enabled);
  void setSaveEnabled(bool enabled);

  std::string m_outputWsName;
  // The ui form
  Ui::IndirectMolDyn m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt
