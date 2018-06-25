#ifndef MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_

#include "DllConfig.h"
#include "ui_ReflSettingsTabWidget.h"
#include "IReflSettingsTabView.h"
#include "IReflSettingsTabPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {


/** QtReflSettingsTabView : Provides an interface for the "Settings" tab in the
ISIS Reflectometry interface.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_ISISREFLECTOMETRY_DLL QtReflSettingsTabView : public QWidget, public IReflSettingsTabView {
  Q_OBJECT
public:
  QtReflSettingsTabView(QWidget *parent = nullptr);
private:
  /// Initialise the interface
  void initLayout();
  void subscribe(IReflSettingsTabPresenter* notifyee) override;

  Ui::ReflSettingsTabWidget m_ui;
  IReflSettingsTabPresenter* m_notifyee;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_ */
