#ifndef MANTID_CUSTOMINTERFACES_QTREFLEVENTTABVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLEVENTTABVIEW_H_

#include "DllConfig.h"
#include "ui_ReflEventTabWidget.h"
#include "IReflEventTabView.h"
#include "IReflEventTabPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

/** QtReflEventTabView : Provides an interface for the "Event" tab in the
Reflectometry interface.

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
class MANTIDQT_ISISREFLECTOMETRY_DLL QtReflEventTabView : public QWidget, public IReflEventTabView {
  Q_OBJECT
public:
  QtReflEventTabView(QWidget *parent = nullptr);
  void subscribe(IReflEventTabPresenter* notifyee);

private:
  void initLayout();
  Ui::ReflEventTabWidget m_ui;
  IReflEventTabPresenter* m_notifyee;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLEVENTTABVIEW_H_ */
