#ifndef MANTIDQT_CUSTOMINTERFACES_SANSBACKGROUNDCORRECTIONWIDGET_H_
#define MANTIDQT_CUSTOMINTERFACES_SANSBACKGROUNDCORRECTIONWIDGET_H_

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

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_ISISSANS_DLL SANSBackgroundCorrectionWidget : public QWidget {
  Q_OBJECT
public:
  SANSBackgroundCorrectionWidget(QWidget *parent = nullptr);

  void
  setDarkRunSettingForTimeDetectors(SANSBackgroundCorrectionSettings setting);
  SANSBackgroundCorrectionSettings getDarkRunSettingForTimeDetectors();
  void
  setDarkRunSettingForUampDetectors(SANSBackgroundCorrectionSettings setting);
  SANSBackgroundCorrectionSettings getDarkRunSettingForUampDetectors();

  void
  setDarkRunSettingForTimeMonitors(SANSBackgroundCorrectionSettings setting);
  SANSBackgroundCorrectionSettings getDarkRunSettingForTimeMonitors();
  void
  setDarkRunSettingForUampMonitors(SANSBackgroundCorrectionSettings setting);
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

#endif /*  MANTIDQT_CUSTOMINTERFACES_SANSBACKGROUNDCORRECTIONWIDGET_H_ */
