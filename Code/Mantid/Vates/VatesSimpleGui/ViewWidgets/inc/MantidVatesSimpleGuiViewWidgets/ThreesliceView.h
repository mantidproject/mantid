#ifndef THREESLICEVIEW_H_
#define THREESLICEVIEW_H_

#include "ui_ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QPointer>
#include <QWidget>

class pqRenderView;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

  class RebinnedSourcesManager;
/**
 *
 This class creates four views of the given dataset. There are three 2D views
 for the three orthogonal Cartesian planes and one 3D view of the dataset
 showing the planes.

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ThreeSliceView : public ViewBase
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget for the threeslice view
   */
  ThreeSliceView(QWidget *parent = 0, RebinnedSourcesManager* rebinnedSourcesManager = 0);
  /// Default destructor.
  virtual ~ThreeSliceView();

  /// Correct the color scale range if not in automatic mode.
  void correctColorScaleRange();
  /**
   * Correct an oddity in the creation of the 3D view so that the cuts
   * are visibile.
   */
  //void correctVisibility();
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

private:
  Q_DISABLE_COPY(ThreeSliceView)

  /// Helper function that creates all three Cartesian orthogonal slices.
  void makeThreeSlice();

  QPointer<pqRenderView> mainView; ///< The 3D view

  Ui::ThreeSliceView ui; ///< The three slice view's UI form
};

}
}
}

#endif // THREESLICEVIEW_H_
