// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidKernel/System.h"
#include "ui_SANSBackgroundCorrectionWidget.h"

#include <QWidget>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
class SANSBackgroundCorrectionSettings;

/** SANSBackgroundCorrectionWidget: Widget for background correction of SANS
experiments.
*/
class MANTIDQT_ISISSANS_DLL SANSBackgroundCorrectionWidget : public QWidget {
  Q_OBJECT
public:
  SANSBackgroundCorrectionWidget(QWidget *parent = nullptr);

  void setDarkRunSettingForTimeDetectors(const SANSBackgroundCorrectionSettings &setting);
  SANSBackgroundCorrectionSettings getDarkRunSettingForTimeDetectors();
  void setDarkRunSettingForUampDetectors(const SANSBackgroundCorrectionSettings &setting);
  SANSBackgroundCorrectionSettings getDarkRunSettingForUampDetectors();

  void setDarkRunSettingForTimeMonitors(const SANSBackgroundCorrectionSettings &setting);
  SANSBackgroundCorrectionSettings getDarkRunSettingForTimeMonitors();
  void setDarkRunSettingForUampMonitors(const SANSBackgroundCorrectionSettings &setting);
  SANSBackgroundCorrectionSettings getDarkRunSettingForUampMonitors();

  void resetEntries();

private slots:
  void handleTimeDetectorsOnOff(int state);
  void handleUampDetectorsOnOff(int state);
  void handleTimeMonitorsOnOff(int state);
  void handleUampMonitorsOnOff(int state);

private:
  /// Setup the connections
  void setupConnections();

  /// UI form
  Ui::SANSBackgroundCorrectionWidget m_ui;
};

} // namespace CustomInterfaces
} // namespace MantidQt
