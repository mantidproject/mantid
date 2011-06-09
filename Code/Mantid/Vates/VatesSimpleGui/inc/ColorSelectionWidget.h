#ifndef COLORSELECTIONWIDGET_H_
#define COLORSELECTIONWIDGET_H_

#include <QtGui/QWidget>
#include "ui_ColorSelectionWidget.h"
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
class ColorSelectionWidget : public QWidget
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget of the mode control widget
   */
  ColorSelectionWidget(QWidget *parent = 0);
  /// Default destructor.
  virtual ~ColorSelectionWidget() {}

protected slots:
  /**
   * Function that enables or diables the min and max line edits based on
   * state of the automatic scaling checkbox.
   *
   * @param state the current state of the checkbox
   */
  void autoOrManualScaling(int state);

private:
  Ui::ColorSelectionWidgetClass ui; ///< The mode control widget's UI form
};

#endif // COLORSELECTIONWIDGET_H_
