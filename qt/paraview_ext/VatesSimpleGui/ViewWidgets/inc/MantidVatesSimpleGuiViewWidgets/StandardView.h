#ifndef STANDARDVIEW_H_
#define STANDARDVIEW_H_

#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "ui_StandardView.h"

#include <QMap>
#include <QPointer>
#include <QWidget>

class pqPipelineSource;
class pqRenderView;
class QAction;

namespace Mantid {
namespace Vates {
namespace SimpleGui {

class RebinnedSourcesManager;

/**
 *
 This class represents the initial view for the main program. It is meant to
 be a view to play with the data in an unstructured manner.

 @date 24/05/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS StandardView
    : public ViewBase {
  Q_OBJECT

public:
  /// Default constructor.
  StandardView(QWidget *parent = nullptr,
               RebinnedSourcesManager *rebinnedSourcesManager = nullptr,
               bool createRenderProxy = true);
  /// Default destructor.
  ~StandardView() override;

  /// @see ViewBase::destroyView
  void destroyView() override;
  /// @see ViewBase::getView
  pqRenderView *getView() override;
  /// @see ViewBase::render
  void render() override;
  /// @see ViewBase::renderAll
  void renderAll() override;
  /// @see ViewBase::resetCamera()
  void resetCamera() override;
  /// @see ViewBase::resetDisplay()
  void resetDisplay() override;
  /// @see ViewBase::updateUI()
  void updateUI() override;
  /// @see ViewBase::updateView()
  void updateView() override;
  /// @see ViewBase::closeSubWindows
  void closeSubWindows() override;
  /// @see ViewBase::setView
  void setView(pqRenderView *view) override;
  /// @see ViewBase::getViewType
  ModeControlWidget::Views getViewType() override;

public slots:
  /// Listen to a change in the active source.
  void activeSourceChangeListener(pqPipelineSource *source);

protected slots:
  /// Add a slice to the current dataset.
  void onCutButtonClicked();
  /// Apply the threshold filter to the current dataset.
  void onThresholdButtonClicked();
  /// Perform operations when rendering is done.
  void onRenderDone();
  /// Invoke the ScaleWorkspace on the current dataset.
  void onScaleButtonClicked();
  /// On BinMD button clicked
  void onRebin();
  /// On scale completed
  void onScaleRepresentationAdded(pqPipelineSource *, pqDataRepresentation *,
                                  int);

private:
  Q_DISABLE_COPY(StandardView)

  QString getAlgNameFromMenuLabel(const QString &menuLbl);

  bool m_cameraReset;
  QPointer<pqPipelineSource> m_scaler; ///< Holder for the ScaleWorkspace
  Ui::StandardView m_ui;               ///< The standard view's UI form
  QPointer<pqRenderView> m_view;       ///< The main view

  /// Set the rebin and unbin button visibility
  void setRebinAndUnbinButtons();
  /// Set up the buttons
  void setupViewButtons();
  ///  Give the user the ability to rebin
  void allowRebinningOptions(bool allow);
  ///  Allow the user the ability to unbin
  void allowUnbinOption(bool allow);

  QAction *m_binMDAction;
  QAction *m_sliceMDAction;
  QAction *m_cutMDAction;
  QAction *m_unbinAction;

  // name to show for the rebin actions on the rebin menu
  static QString g_binMDName;
  static QString g_sliceMDName;
  static QString g_cutMDName;
  /// name + a bit of description
  static QString g_binMDLbl;
  static QString g_sliceMDLbl;
  static QString g_cutMDLbl;
  /// tool tip text for the rebin algorithm
  static QString g_binMDToolTipTxt;
  static QString g_sliceMDToolTipTxt;
  static QString g_cutMDToolTipTxt;
  static QMap<QString, QString> g_actionToAlgName;
  /// wire frame representation key
  static const std::string SurfaceRepresentation;
  /// surface representation key
  static const std::string WireFrameRepresentation;
};

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // STANDARDVIEW_H_
