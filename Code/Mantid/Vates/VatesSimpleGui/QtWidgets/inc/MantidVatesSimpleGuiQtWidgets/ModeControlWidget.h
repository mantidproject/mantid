#ifndef MODECONTROLWIDGET_H_
#define MODECONTROLWIDGET_H_

#include "ui_ModeControlWidget.h"
#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"

#include <map>
#include <QWidget>
#include <QString>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
 *
  This class controls the current view for the main level program.

  @author Michael Reuter
  @date 24/05/2011

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS ModeControlWidget : public QWidget
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
  enum Views {STANDARD, THREESLICE, MULTISLICE, SPLATTERPLOT};

public slots:
  /// Enable/disable a specific view button.
  void enableViewButton(ModeControlWidget::Views mode, bool state);

  /**
   * Enable/disable all view buttons, except standard.
   * @param state whether or not to enable the buttons
   * @param initialView The initial view.
   */
  void enableViewButtons(ModeControlWidget::Views initialView, bool state);

  /// Expose the standard view button.
  void setToStandardView();

  /// Switch to a selected view.
  void setToSelectedView(ModeControlWidget::Views view);

  /// Convert a string into an enum
  ModeControlWidget::Views getViewFromString(QString view);

signals:
  /**
   * Function to make the main program window switch to a given view.
   * @param v the type of view to switch to
   */
  void executeSwitchViews(ModeControlWidget::Views v);

protected slots:
  /**
   * Execute switch to multislice view, disable multislice button and
   * enable other view buttons.
   */
  void onMultiSliceViewButtonClicked();
  /**
   * Execute switch to splatter plot view, disable splatter plot
   * button and enable other views.
   */
  void onSplatterPlotViewButtonClicked();
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
  std::map<QString, Views> mapFromStringToView; //< Holds the mapping from the a string to an associated enum
};

}
}
}

#endif // MODECONTROLWIDGET_H_
