#ifndef ROTATIONPOINTDIALOG_H
#define ROTATIONPOINTDIALOG_H

#include "ui_RotationPointDialog.h"
#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"

#include <QDialog>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
 *
  This class handles providing the coordinates for a center of rotation.

  @date 14/11/2011

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS RotationPointDialog : public QDialog
{
    Q_OBJECT

public:
  /// Default constructor.
  RotationPointDialog(QWidget *parent = 0);
  /// Default destructor.
  ~RotationPointDialog();

signals:
  /**
   * Signal to pass along the individual coordinate values.
   * @param x the x coordinate of the point
   * @param y the y coordinate of the point
   * @param z the z coordinate of the point
   */
  void sendCoordinates(double x, double y, double z);

protected slots:
  /// Gather the coordinates from the dialog line editors.
  void getCoordinates();

private:
  Ui::RotationPointDialog ui; ///< The dialog's UI form
};

}
}
}

#endif // ROTATIONPOINTDIALOG_H
