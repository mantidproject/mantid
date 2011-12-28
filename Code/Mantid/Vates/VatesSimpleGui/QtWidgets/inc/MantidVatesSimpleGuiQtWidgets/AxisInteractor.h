#ifndef AXISINTERACTOR_H_
#define AXISINTERACTOR_H_

#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"

#include <QWidget>

class QBoxLayout;
class QGraphicsScene;
class QGraphicsView;
class QMenu;
class QMouseEvent;
class QString;
class QwtScaleEngine;
class QwtScaleTransformation;
class QwtScaleWidget;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

class AxisInformation;
class ScalePicker;

/**
 *
  This class provides a mechanism for setting slices onto a dataset that are
  associated with an individual dataset axis. The slice indicators are
  represented by triangles pointing at their current location via the
  associated axis widget. A new slice and indicator is achieved by
  right-clicking on the empty space near, but not on, the axis widget.

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS AxisInteractor : public QWidget
{
  Q_OBJECT
  Q_ENUMS(ScalePos)
  Q_PROPERTY( ScalePos scalePosition READ scalePosition
              WRITE setScalePosition )
  Q_PROPERTY(double getMinimum READ getMinimum )
  Q_PROPERTY(double getMaximum READ getMaximum )
  Q_PROPERTY(QString getTitle READ getTitle )

public:
  /// Enumeration for scale orientation
  enum ScalePos {
    LeftScale,
    RightScale,
    TopScale,
    BottomScale };
  /**
   * Default constructor.
   * @param parent the parent UI object for the axis interactor widget
   */
  AxisInteractor(QWidget *parent = 0);
  /// Default destructor.
  virtual ~AxisInteractor() {}
  /**
   * Remove highlights from all selected indicators.
   */
  void clearSelections();
  /// Delete all of the indicators.
  void deleteAllIndicators();
  /// Delete the requested indicator.
  void deleteRequestedIndicator(const QString &name);
  /**
   * Get the associated ScalePicker for the indicator.
   * @return the associated ScalePicker
   */
  ScalePicker *getScalePicker() { return this->scalePicker; }
  /// Get the axis scale maximum.
  double getMaximum();
  /// Get the axis scale minimum.
  double getMinimum();
  /// Get the axis title.
  QString getTitle();
  /**
   * Is there at least one indicator?
   * @return true if yes
   */
  bool hasIndicator();
  /**
   * Get the number of indicators held by the object.
   * @return the current number of indicators
   */
  int numIndicators();
  /**
   * Return the orientation of the axis scale ticket marks
   * @return the orientation code
   */
  ScalePos scalePosition() const;
  /**
   * Highlight the requested indicator.
   * @param name the name of the slice being highlighted
   */
  void selectIndicator(const QString &name);
  /// Set the bounds for the axis scale.
  void setBounds(AxisInformation *info, bool update=false);
  /// Set the axis information.
  void setInformation(AxisInformation *info, bool update=false);
  /**
   * Set the orientation of the axis scale and graphicsview.
   * @param orient the orientation of the graphicsview
   * @param scalePos the orientation of the axis scale
   */
  void setOrientation(Qt::Orientation orient, ScalePos scalePos);
  /**
   * Set the orientation of the axis scale tick marks.
   * @param scalePos the orientation code
   */
  void setScalePosition(ScalePos scalePos);
  /**
   * Update the current indicator to a new location.
   * @param value the new location for the indicator
   */
  /// Set the flag for showing the SliceViewer.
  void setShowSliceView(double state);
  /// Update the indicator to the value requested.
  void updateIndicator(double value);
  /// Update the requested indicator to the given position.
  void updateRequestedIndicator(const QString &name, double value);
  /// Update the scene rectangle for the graphics view.
  void updateSceneRect();

signals:
  /**
   * Signal to pass along the name of the indicator to delete.
   * @param name the name of the indicator to be deleted
   */
  void deleteIndicator(const QString &name);
  /**
   * Signal to pass along the name of the indicator being selected.
   * @param name the name of the selected indicator
   */
  void indicatorSelected(const QString &name);
  /**
   * Signal to pass the name of the slice to open up in the SliceViewer.
   * @param name the name of the selected indicator
   */
  void showInSliceView(const QString &name);
  /**
   * Signal to show or hide the given indicator.
   * @param isVisible flag the determines showing or hiding the indicator
   * @param name the name of the indicator to show or hide
   */
  void showOrHideIndicator(bool isVisible, const QString &name);

protected slots:
  /**
   * Create an indicator at the requested location that is associated with
   * a new slice.
   * @param point the (x,y) location for the indicator
   */
  void createIndicator(const QPoint &point);
  /// Determine the indicator being selected and pass along that information.
  void getIndicator();
  /**
   * Associate a ParaView slice object name with the new indicator.
   * @param name the ParaView name of the slice
   */
  void setIndicatorName(const QString &name);
  /**
   * Show a context menu for the indicator that will allow it to be
   * deleted or hidden/shown.
   * @param pos location to show the context menu
   */
  void showContextMenu(const QPoint &pos);

private:
  /// Create the context menu of the indicators
  void createContextMenu();
  /// Handle the setup of the widget based on orientation requests.
  void widgetLayout();

  bool canShowSliceView; ///< Can view show SliceViewer
  QMenu *indicatorContextMenu; ///< The indicator context menu
  QwtScaleEngine *engine; ///< The scale type for the axis widget
  QGraphicsView *graphicsView; ///< The holder for the slice indicators
  QBoxLayout *boxLayout; ///< Layout manager for widgets
  bool isSceneGeomInit; ///< Flag to ensure the scene is initialized once
  Qt::Orientation orientation; ///< The overall orientation of the widget
  ScalePicker *scalePicker; ///< The picker that retrieves the axis location
  ScalePos scalePos; ///< The orientation of the axis scale tick marks
  QwtScaleWidget *scaleWidget; ///< The axis scale widget
  QGraphicsScene *scene; ///< The contained for the slice indicators
  QwtScaleTransformation *transform; ///< The scale type for the engine
};

}
}
}

#endif // AXISINTERACTOR_H_
