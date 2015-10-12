#ifndef MANTIDQT_API_SYNCEDCHECKBOXES_H_
#define MANTIDQT_API_SYNCEDCHECKBOXES_H_

#include "MantidKernel/System.h"
#include "DllOption.h"
#include <QtGui/qwidget.h>
#include <qpushbutton.h>
#include <QtGui/qaction.h>

namespace MantidQt
{
namespace API
{

  /** QObject that links:

    - a checkable QAction (menu item)
    - and a checkable QAbstractButton (like a toolbar button)

    so that their checked status is consistent.
    Emits a single "toggled" signal if either one is toggled.
    
    @date 2011-12-06

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class EXPORT_OPT_MANTIDQT_API SyncedCheckboxes : public QObject
  {
    Q_OBJECT

  public:
    SyncedCheckboxes(QAction * menu, QAbstractButton * button, bool checked = false);
    virtual ~SyncedCheckboxes();
    void toggle(bool val);
    void setEnabled(bool val);
    void setVisible(bool val);
    
  signals:
    /// Signal emitted when the check box is toggled
    void toggled(bool);

  public slots:
    void on_menu_toggled(bool);
    void on_button_toggled(bool);

  private:
    QAction * m_menu;
    QAbstractButton * m_button;
  };


} // namespace API
} // namespace MantidQt

#endif  /* MANTIDQT_API_SYNCEDCHECKBOXES_H_ */
