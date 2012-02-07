#ifndef MDVIEWERWIDGET_H_
#define MDVIEWERWIDGET_H_

#include "ui_MdViewerWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include "MantidQtAPI/VatesViewerInterface.h"

#include <QPointer>
#include <QWidget>

class pqLoadDataReaction;
class pqPipelineSource;
class vtkSMDoubleVectorProperty;

class QAction;
class QEvent;
class QHBoxLayout;
class QObject;
class QString;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

class ColorSelectionDialog;
class RotationPointDialog;
class ViewBase;

/**
 *
  This class represents the central widget for handling VATES visualization
  operations for 3D and 4D datasets.

  @date 11/08/2011

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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS MdViewerWidget : public MantidQt::API::VatesViewerInterface
{
  Q_OBJECT

public:
  /// Plugin mode constructor.
  MdViewerWidget();
  /// Standalone mode constructor.
  MdViewerWidget(QWidget *parent);
  /// Default destructor.
  virtual ~MdViewerWidget();

  /// Add extra menus for standalone mode.
  void addMenus();
  /// Connect data loader.
  void connectLoadDataReaction(QAction *action);
  /// Filter events to check for hide.
  bool eventFilter(QObject *obj, QEvent *ev);
  /// See MantidQt::API::VatesViewerInterface
  void renderWorkspace(QString wsname, int wstype);
  /// See MantidQt::API::VatesViewerInterface
  void setupPluginMode();

protected slots:
  /// Check for certain updates when an accept is fired.
  void checkForUpdates();
  /// Pop-up the color options dialog.
  void onColorOptions();
  /// Pop-up the rotation point dialog.
  void onRotationPoint();
  /// Show the wiki help in a browser.
  void onWikiHelp();
  /// Load and render data.
  void onDataLoaded(pqPipelineSource *source);
  /// Execute view switch.
  void switchViews(ModeControlWidget::Views v);

private:
  Q_DISABLE_COPY(MdViewerWidget)

  ColorSelectionDialog *colorDialog; ///< Holder for the color options dialog
  ViewBase *currentView; ///< Holder for the current view
  pqLoadDataReaction *dataLoader; ///< Holder for the load data reaction
  ViewBase *hiddenView; ///< Holder for the view that is being switched from
  bool isPluginInitialized; ///< Flag for plugin initialization
  bool pluginMode; ///< Flag to say widget is in plugin mode
  RotationPointDialog *rotPointDialog; ///< Holder for the rotation point dialog
  Ui::MdViewerWidgetClass ui; ///< The MD viewer's UI form
  QHBoxLayout *viewLayout; ///< Layout manager for the view widget

  /// Check the environmental variables.
  void checkEnvSetup();
  /// Setup color options dialog connections.
  void connectColorOptionsDialog();
  /// Setup connections for all dialogs.
  void connectDialogs();
  /// Setup rotation point dialog connections.
  void connectRotationPointDialog();
  /// Create the pqPVApplicationCore object in plugin mode.
  void createAppCoreForPlugin();
  /// Add view specific stuff to a menu.
  void createMenus();
  /// Disconnect dialog connections.
  void disconnectDialogs();
  /// Consolidate constructor related items.
  void internalSetup(bool pMode);
  /// Disable communication with the proxy tab widget.
  void removeProxyTabWidgetConnections();
  /// Perform first render and final setup for mode buttons.
  void renderAndFinalSetup();
  /// Set the signals/slots for the ParaView components based on the view.
  void setParaViewComponentsForView();
  /// Run the necessary setup for the main view.
  void setupMainView();
  /// Mimic ParaView behavior setup without QMainWindow.
  void setupParaViewBehaviors();
  /// Creates the UI and mode switch connection.
  void setupUiAndConnections();
  /// Create the requested view.
  ViewBase *setMainViewWidget(QWidget *container, ModeControlWidget::Views v);
  /// Helper function to swap current and hidden view pointers.
  void swapViews();
};

} // SimpleGui
} // Vates
} // Mantid

#endif // MDVIEWERWIDGET_H_
