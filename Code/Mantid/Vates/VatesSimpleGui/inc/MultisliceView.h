#ifndef MULTISLICEVIEW_H_
#define MULTISLICEVIEW_H_

#include <QtGui/QWidget>
#include <QPointer>
#include "ViewBase.h"
#include "ui_MultisliceView.h"

class pqColorMapModel;
class pqPipelineRepresentation;
class pqPipelineSource;
class pqRenderView;

class QModelIndex;
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
class MultiSliceView : public ViewBase
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
   * ViewBase::getView
   */
  pqRenderView* getView();
  /**
   * ViewBase::render
   */
  void render();

protected slots:
  /**
   * Make the slice interactor appear on the view when an indicator is
   * selected.
   * @param name the server manager name of the selected slice
   */
  void indicatorSelected(const QString &name);
  /// Set the color scale back to the original bounds.
  void onAutoScale();
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
  /**
   * Set the requested color map on the data.
   * @param model the color map to use
   */
  void onColorMapChange(const pqColorMapModel *model);
  /**
   * Set the color scale to the currently requested bounds.
   * @param min the minimum value for the color scale
   * @param max the maximum value for the color scale
   */
  void onColorScaleChange(double min, double max);
	/// Select the appropriate indicator on the correct axis interactor widget.
	void selectIndicator();
  void updateCutPosition(double position);
	/// Move the selected indicator to a given position.
	void updateSelectedIndicator();

signals:
  /**
   * Signal to identify the name of a created slice indicator.
   * @param name the name of the ParaView slice representation
   */
	void sliceNamed(const QString &name);
  /**
   * Signal to get the range of the data.
   * @param min the minimum value of the data
   * @param max the maximum value of the data
   */
  void dataRange(double min, double max);

private:
    Q_DISABLE_COPY(MultiSliceView);
    /// Clear all axis indicator highlighting.
    void clearIndicatorSelections();
    /**
     * Clear the selections from the pipeline browser that do not correspond
     * to the currently selected slice.
     * @param name the name of the currently selected slice
     */
    void clearPbwSelections(const QString &name);
    /**
     * Function that creates a slice in the appropriate plane at the
     * requested axis location.
     * @param origin the cartesian coordinates of the slice origin
     * @param orient the cartesian coordinates of the slice normal
     */
    void makeCut(double origin[], double orient[]);
    /// Pull the dataset information and setup the axis interactors.
    void setupAxisInfo();
    /// Create the current data representation.
    void setupData();

    QPointer<pqRenderView> mainView; ///< The main view class
    QPointer<pqPipelineSource> origSource; ///< The current source
    QPointer<pqPipelineRepresentation> originSourceRepr; ///< The current source representation
    Ui::MultiSliceViewClass ui; ///< The view's UI form
};

#endif // MULTISLICEVIEW_H_
