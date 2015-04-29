#ifndef STANDARDVIEW_H_
#define STANDARDVIEW_H_

#include "ui_StandardView.h"
#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QPointer>
#include <QWidget>

class pqPipelineSource;
class pqRenderView;
class QAction;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

  class RebinnedSourcesManager;
/**
 *
 This class represents the initial view for the main program. It is meant to
 be a view to play with the data in an unstructured manner.

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS StandardView : public ViewBase
{
  Q_OBJECT

public:
  /// Default constructor.
  StandardView(QWidget *parent = 0, RebinnedSourcesManager* rebinnedSourcesManager = 0);
  /// Default destructor.
  virtual ~StandardView();

  /// @see ViewBase::destroyView
  void destroyView();
  /// @see ViewBase::getView
  pqRenderView* getView();
  /// @see ViewBase::render
  void render();
  /// @see ViewBase::renderAll
  void renderAll();
  /// @see ViewBase::resetCamera()
  void resetCamera();
  /// @see ViewBase::resetDisplay()
  void resetDisplay();
  /// @see ViewBase::updateUI()
  void updateUI();
  /// @see ViewBase::updateView()
  void updateView();
  /// @see ViewBase::closeSubWindows
  void closeSubWindows();

public slots:
  /// Listen to a change in the active source.
  void activeSourceChangeListener(pqPipelineSource* source);

protected slots:
  /// Add a slice to the current dataset.
  void onCutButtonClicked();
  /// Perform operations when rendering is done.
  void onRenderDone();
  /// Invoke the ScaleWorkspace on the current dataset.
  void onScaleButtonClicked();
  /// On BinMD button clicked
  void onRebin();

private:
  Q_DISABLE_COPY(StandardView)

  bool cameraReset;
  QPointer<pqPipelineSource> scaler; ///< Holder for the ScaleWorkspace
  Ui::StandardView ui; ///< The standard view's UI form
  QPointer<pqRenderView> view; ///< The main view

  /// Set the rebin and unbin button visibility
  void setRebinAndUnbinButtons();
  /// Set up the buttons
  void setupViewButtons();
  ///  Give the user the ability to rebin
  void allowRebinningOptions(bool allow);
  ///  Allow the user the ability to unbin
  void allowUnbinOption(bool allow);

  QAction* m_binMDAction;
  QAction* m_sliceMDAction;
  QAction* m_cutMDAction;
  QAction* m_unbinAction;
};

} // SimpleGui
} // Vates
} // Mantid

#endif // STANDARDVIEW_H_
