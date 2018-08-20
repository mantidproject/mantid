#ifndef COLORMAPEDITORPANEL_H
#define COLORMAPEDITORPANEL_H

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "ui_ColorMapEditorPanel.h"
#include <QDialog>
#include <QWidget>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
  This class handles the color map editor.

  @date 11/05/2015

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ColorMapEditorPanel
    : public QDialog {
  Q_OBJECT
public:
  /// Default constructor.
  ColorMapEditorPanel(QWidget *parent = nullptr);
  /// Default destructor.
  ~ColorMapEditorPanel() override;
  /// Connect the panel to ParaView
  void setUpPanel();
  /// Filter events to check for show events.
  bool eventFilter(QObject *obj, QEvent *ev) override;

signals:
  void showPopUpWindow();
  void hidePopUpWindow();

public slots:
  // Show the window pop up
  void onShowPopUpWindow();
  // Hide the pop up window
  void onHidePopUpWindow();

private:
  Ui::ColorMapEditorPanel ui; ///< The dialog's UI form
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // COLORMAPEDITORPANEL_H
