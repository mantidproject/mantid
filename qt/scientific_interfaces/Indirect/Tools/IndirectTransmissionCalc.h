// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "IndirectToolsTab.h"
#include "MantidAPI/ExperimentInfo.h"
#include "ui_IndirectTransmissionCalc.h"

#include <QComboBox>
#include <QMap>
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL IndirectTransmissionCalc : public IndirectToolsTab {
  Q_OBJECT

public:
  IndirectTransmissionCalc(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

protected:
  void setup() override;
  bool validate() override;
  void run() override;

private slots:
  /// Handles completion of the algorithm
  void algorithmComplete(bool error);
  void runClicked();

private:
  void setRunIsRunning(bool running);
  void setRunEnabled(bool enabled);

  /// The UI form
  Ui::IndirectTransmissionCalc m_uiForm;
  /// The name of the current instrument
  QString m_instrument;
};
} // namespace CustomInterfaces
} // namespace MantidQt
