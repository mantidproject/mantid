#ifndef COLORSELECTIONDIALOG_H_
#define COLORSELECTIONDIALOG_H_

#include "ui_ColorSelectionDialog.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QDialog>

class pqColorMapModel;
class pqColorPresetManager;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
 *
  This class controls the color scale for the main level program viewed
  datasets.

  @author Michael Reuter
  @date 07/06/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ColorSelectionDialog : public QDialog
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget of the mode control widget
   */
  ColorSelectionDialog(QWidget *parent = 0);
  /// Default destructor.
  virtual ~ColorSelectionDialog() {}

public slots:
  /**
   * Function to set the color scale range into the range widgets.
   *
   * @param min the minimum value of the color scale range
   * @param max the maximum value of the color scale range
   */
  void setColorScaleRange(double min, double max);

signals:
  /**
   * Signal to let views know that autoscaling is on.
   */
  void autoScale();
  /**
   * Signal to pass on information about a change to the color map.
   * @param model the color map to send
   */
  void colorMapChanged(const pqColorMapModel *model);
  /**
   * Signal to pass on information that the color scale has changed.
   *
   * @param min the minimum value of the color scale
   * @param max the maximum value of the color scale
   */
  void colorScaleChanged(double min, double max);
  /**
   * Signal to pass on use of log color scaling.
   * @param state flag for whether or not to use log color scaling
   */
  void logScale(int state);

protected slots:
  /**
   * Function that enables or diables the min and max line edits based on
   * state of the automatic scaling checkbox.
   *
   * @param state the current state of the checkbox
   */
  void autoOrManualScaling(int state);
  /**
   * Function to get the new color scale range.
   */
  void getColorScaleRange();
  /**
   * Function that presents the user with the available color presets and 
   * gets the
   * result from the user.
   */
  void loadPreset();
  /**
   * Function that sets the flag for using log color scaling based on the
   * associated checkbox.
   * @param state flag for whether or not to use log color scaling
   */
  void useLogScaling(int state);

private:
  /**
   * Function that sets up various colormaps. This is copied verbaitum from 
   * pqColorScaleEditor.
   */
  void loadBuiltinColorPresets();
  /**
   * Function that sets the status of the editor widgets.
   *
   * @param status the state to set the editor widgets to
   */
  void setEditorStatus(bool status);

  pqColorPresetManager *presets; ///< Dialog for choosing color presets
  Ui::ColorSelectionDialogClass ui; ///< The mode control widget's UI form
};

}
}
}

#endif // COLORSELECTIONWIDGET_H_
