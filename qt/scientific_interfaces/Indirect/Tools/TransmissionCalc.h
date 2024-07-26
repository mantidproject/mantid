// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ToolsTab.h"
#include "ui_TransmissionCalc.h"

#include <QComboBox>
#include <QMap>
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INDIRECT_DLL TransmissionCalc : public ToolsTab {
  Q_OBJECT

public:
  TransmissionCalc(QWidget *parent = nullptr);

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
  Ui::TransmissionCalc m_uiForm;
  /// The name of the current instrument
  QString m_instrument;
};
} // namespace CustomInterfaces
} // namespace MantidQt
