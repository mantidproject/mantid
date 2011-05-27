#ifndef MULTISLICEVIEW_H
#define MULTISLICEVIEW_H

#include <QtGui/QWidget>
#include <QPointer>
#include "IView.h"
#include "ui_MultisliceView.h"

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
class MultiSliceView : public IView
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
     * IView::getView
     */
    pqRenderView* getView();
    /**
     * IView::render
     */
    void render();

protected slots:
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
	/// Move the selected indicator to a given position.
	void updateSelectedIndicator();

signals:
  /**
   * Signal to identifiy the name of a created slice indicator.
   * @param name the name of the ParaView slice representation
   */
	void sliceNamed(const QString &name);

private:
    Q_DISABLE_COPY(MultiSliceView);
    /// Clear all axis indicator highlighting.
    void clearIndicatorSelections();
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

#endif // MULTISLICEVIEW_H
