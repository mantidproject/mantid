#ifndef MODECONTROLWIDGET_H
#define MODECONTROLWIDGET_H

#include <QtGui/QWidget>
#include "ui_ModeControlWidget.h"
/**
 *
  This class controls the current view for the main level program.

  @author Michael Reuter
  @date 24/05/2011

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
class ModeControlWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Default constructor.
     * @param parent the parent widget of the mode control widget
     */
    ModeControlWidget(QWidget *parent = 0);
    /// Default destructor.
    virtual ~ModeControlWidget();

    /// Enumeration for the view types
    enum Views {STANDARD, THREESLICE, MULTISLICE};

signals:
  /**
   * Function to make the main program window switch to a given view.
   * @param v the type of view to switch to
   */
	void executeSwitchViews(ModeControlWidget::Views v);

protected slots:
  /**
   * Enable on the threeslice and multislice view buttons.
   */
	void enableModeButtons();
	/**
	 * Execute switch to multislice view, disable multislice button and
	 * enable other view buttons.
	 */
	void onMultiSliceViewButtonClicked();
  /**
   * Execute switch to standard view, disable standard button and
   * enable other view buttons.
   */
  void onStandardViewButtonClicked();
  /**
   * Execute switch to threeslice view, disable threeslice button and
   * enable other view buttons.
   */
  void onThreeSliceViewButtonClicked();

private:
    Ui::ModeControlWidgetClass ui; ///< The mode control widget's UI form
};

#endif // MODECONTROLWIDGET_H
