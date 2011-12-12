#ifndef MULTISLICEVIEW_H_
#define MULTISLICEVIEW_H_

#include "ui_MultisliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QPointer>
#include <QWidget>

class pqColorMapModel;
class pqPipelineRepresentation;
class pqPipelineSource;
class pqRenderView;

class QModelIndex;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

class AxisInformation;
class AxisInteractor;
/**
 *
  This class creates a multislice view which is based on the Matlab(C)
  SliceOMatic view. This view is specfically designed for 3(+1)D datasets.

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS MultiSliceView : public ViewBase
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget of the multislice view widget
   */
  MultiSliceView(QWidget *parent = 0);
  /// Default constructor.
  virtual ~MultiSliceView();

  /**
   * ViewBase::destroyView
   */
  void destroyView();
  /**
   * ViewBase::getView
   */
  pqRenderView* getView();
  /**
   * ViewBase::render
   */
  void render();
  /**
   * ViewBase::renderAll
   */
  void renderAll();
  /// ViewBase::resetCamera()
  void resetCamera();
  /**
   * ViewBase::resetDisplay()
   */
  void resetDisplay();
  /// ViewBase::setAxisScales()
  void setAxisScales();

protected slots:
  /**
   * Show or hide a given slice from the view.
   * @param isVisible flag to show/hide associated cut
   * @param name the slice to show/hide from the view
   */
  void cutVisibility(bool isVisible, const QString &name);
  /**
   * Delete a slice from the view.
   * @param name the name of the slice to be deleted
   */
  void deleteCut(const QString &name);
  /**
   * Make the slice interactor appear on the view when an indicator is
   * selected.
   * @param name the server manager name of the selected slice
   */
  void indicatorSelected(const QString &name);
  /**
   * Create a slice in the YZ plane at a specific point on the dataset x axis.
   * @param value create a YZ slice at the given x axis location
   */
	void makeXcut(double value);
  /**
   * Create a slice in the XZ plane at a specific point on the dataset y axis.
   * @param value create a XZ slice at the given y axis location
   */
	void makeYcut(double value);
  /**
   * Create a slice in the XY plane at a specific point on the dataset z axis.
   * @param value create a XY slice at the given z axis location
   */
	void makeZcut(double value);
	/// Select the appropriate indicator on the correct axis interactor widget.
	void selectIndicator();
  /**
   * Update the origin position of the currently selected cut.
   * @param position the origin coordinate to move the emitting slice to
   */
  void updateCutPosition(double position);
	/// Move the selected indicator to a given position.
	void updateSelectedIndicator();

signals:
  /**
   * Signal to identify the name of a created slice indicator.
   * @param name the name of the ParaView slice representation
   */
	void sliceNamed(const QString &name);

private:
  Q_DISABLE_COPY(MultiSliceView)

  /// Determine if the incoming and current axis have the same bounds.
  bool checkBounds(AxisInformation *info, AxisInteractor *axis);
  /// Determine if the data can support the SliceViewer being shown.
  void checkSliceViewCompat();
  /// Determine if the incoming and current axis have the same title.
  bool checkTitles(AxisInformation *info, AxisInteractor *axis);
  /// Clear all axis indicator highlighting.
  void clearIndicatorSelections();
  /// Filter resize events.
  bool eventFilter(QObject *ob, QEvent *ev);
  /**
   * Function that polls all of the axis indicators to see if any are left.
   * @return true if no indicators are left, false if there are indicators
   */
  bool noIndicatorsLeft();
  /**
   * Function that creates a slice in the appropriate plane at the
   * requested axis location.
   * @param origin the cartesian coordinates of the slice origin
   * @param orient the cartesian coordinates of the slice normal
   */
  void makeCut(double origin[], double orient[]);
  /// Reset or delete indicator when bounds change.
  void resetOrDeleteIndicators(AxisInteractor *axis, int pos);
  /// Pull the dataset information and setup the axis interactors.
  void setupAxisInfo();
  /// Create the current data representation.
  void setupData();

  bool isOrigSrc; ///< Flag for SliceViewer information
  QPointer<pqRenderView> mainView; ///< The main view class
  Ui::MultiSliceViewClass ui; ///< The view's UI form
};

}
}
}

#endif // MULTISLICEVIEW_H_
