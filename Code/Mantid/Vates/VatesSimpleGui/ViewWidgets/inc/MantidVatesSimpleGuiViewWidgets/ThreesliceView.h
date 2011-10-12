#ifndef THREESLICEVIEW_H_
#define THREESLICEVIEW_H_

#include "ui_ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QPointer>
#include <QWidget>

class pqColorMapModel;
class pqPipelineBrowserWidget;
class pqPipelineRepresentation;
class pqPipelineSource;
class pqRenderView;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
 *
 This class creates four views of the given dataset. There are three 2D views
 for the three orthogonal Cartesian planes and one 3D view of the dataset
 showing the planes.

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ThreeSliceView : public ViewBase
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget for the threeslice view
   */
  ThreeSliceView(QWidget *parent = 0);
  /// Default destructor.
  virtual ~ThreeSliceView();

  /// Correct the color scale range if not in automatic mode.
  void correctColorScaleRange();
  /**
   * Correct an oddity in the creation of the 3D view so that the cuts
   * are visibile.
   * @param pbw the main program's handle to the pqPipelineBrowserWidget
   */
  void correctVisibility(pqPipelineBrowserWidget *pbw);
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
  /**
   * ViewBase::resetDisplay()
   */
  void resetDisplay();

protected:
  /**
   * Function that creates a 2D view by reducing the functionality of a 3D
   * view.
   * @param container the widget to associate with the view
   * @return the created view
   */
  pqRenderView *create2dRenderView(QWidget *container);

private:
  Q_DISABLE_COPY(ThreeSliceView)

  /**
   * A helper function that creates a single Cartesian slice.
   * @param i the Cartesian direction of the slice
   * @param view the associated Cartesian 2D view
   * @param cut the created Cartesian slice
   * @param repr the representation of the Cartesian slice
   */
  void makeSlice(ViewBase::Direction i, pqRenderView *view,
                 pqPipelineSource *cut, pqPipelineRepresentation *repr);
  /// Helper function that creates all three Cartesian orthogonal slices.
  void makeThreeSlice();

  QPointer<pqRenderView> mainView; ///< The 3D view
  QPointer<pqPipelineSource> xCut; ///< The slice for the YZ plane
  QPointer<pqPipelineRepresentation> xCutRepr; ///< The YZ slice representation
  QPointer<pqRenderView> xView; ///< The YZ plane view
  QPointer<pqPipelineSource> yCut; ///< The slice for the XZ plane
  QPointer<pqPipelineRepresentation> yCutRepr; ///< The XZ slice representation
  QPointer<pqRenderView> yView; ///< The XZ plane view
  QPointer<pqPipelineSource> zCut; ///< The slice for the XY plane
  QPointer<pqPipelineRepresentation> zCutRepr; ///< The XY slice representation
  QPointer<pqRenderView> zView; ///< The XY plane view

  Ui::ThreeSliceView ui; ///< The three slice view's UI form
};

}
}
}

#endif // THREESLICEVIEW_H_
