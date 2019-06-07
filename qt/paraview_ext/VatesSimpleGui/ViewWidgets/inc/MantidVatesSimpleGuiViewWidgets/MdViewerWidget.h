// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDVIEWERWIDGET_H_
#define MDVIEWERWIDGET_H_

#include "MantidQtWidgets/Common/MdConstants.h"
#include "MantidQtWidgets/Common/MdSettings.h"
#include "MantidQtWidgets/Common/VatesViewerInterface.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidVatesAPI/ColorScaleGuard.h"
#include "MantidVatesSimpleGuiViewWidgets/RebinAlgorithmDialogProvider.h"
#include "MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "ui_MdViewerWidget.h"

#include "vtkSmartPointer.h"

#include "boost/optional.hpp"
#include "boost/shared_ptr.hpp"

// forward declaration of ParaQ classes
class pqApplicationSettingsReaction;
class pqLoadDataReaction;
class pqPipelineSource;
class pqSaveScreenshotReaction;

// forward declaration of Qt classes
class QAction;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QHBoxLayout;
class QObject;
class QWidget;

namespace Mantid {
namespace Vates {
namespace SimpleGui {

class RotationPointDialog;
class ViewBase;
class RebinDialog;
class ColorMapEditorPanel;
/**
*
This class represents the central widget for handling VATES visualization
operations for 3D and 4D datasets.
@date 11/08/2011
*/
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS MdViewerWidget
    : public MantidQt::API::VatesViewerInterface,
      MantidQt::API::WorkspaceObserver {
  Q_OBJECT

public:
  /// Plugin mode constructor.
  MdViewerWidget();
  /// Standalone mode constructor.
  MdViewerWidget(QWidget *parent);
  /// Default destructor.
  ~MdViewerWidget() override;

  /// Add extra menus for standalone mode.
  void addMenus();
  /// Connect data loader.
  void connectLoadDataReaction(QAction *action);
  /// Filter events to check for hide.
  bool eventFilter(QObject *obj, QEvent *ev) override;
  /// See MantidQt::API::VatesViewerInterface
  void renderWorkspace(QString workspaceName, int workspaceType,
                       std::string instrumentName) override;
  /// See MantidQt::API::VatesViewerInterface
  void setupPluginMode(int WsType, const std::string &instrumentName) override;
  /// Load the state of the window from a Mantid project file
  void loadFromProject(const std::string &lines) override;
  /// Save the state of the window to a Mantid project file
  std::string saveToProject(ApplicationWindow *app) override;
  /// Returns a list of workspace names that are used by this window
  std::vector<std::string> getWorkspaceNames() override;
  /// Returns the user friendly name of the window
  std::string getWindowName() override;
  /// Returns the type of the window
  std::string getWindowType() override;

public slots:
  /// See MantidQt::API::VatesViewerInterface
  void shutdown() override;

protected slots:
  /// Check for certain updates when an accept is fired.
  void checkForUpdates();
  /// Turn on/off the LOD threshold.
  void onLodToggled(bool state);
  /// Pop-up the rotation point dialog.
  void onRotationPoint();
  /// Show the wiki help in a browser.
  void onWikiHelp();
  /// Load and render data.
  void onDataLoaded(pqPipelineSource *source);
  /// Perform actions when rendering is done.
  void renderingDone();
  /// Execute view switch.
  void switchViews(ModeControlWidget::Views v);
  /// Triggered when panel is changed.
  void panelChanged();
  /// On rebin
  void onRebin(const std::string &algorithmType);
  /// On  unbin
  void onUnbin();
  /// On switching an MDEvent source to a temporary source.
  void onSwitchSources(std::string rebinnedWorkspaceName,
                       std::string sourceType);
  /// reset state of all the views
  void onResetViewsStateToAllData();
  void showOutputWidget();

protected:
  /// Handle workspace preDeletion tasks.
  void
  preDeleteHandle(const std::string &wsName,
                  const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  /// Handle workspace replacement tasks.
  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  /// Detects if something is dragged onto the VSI
  void dragEnterEvent(QDragEnterEvent *e) override;
  /// Reacts to something being dropped onto the VSI
  void dropEvent(QDropEvent *e) override;

private:
  Q_DISABLE_COPY(MdViewerWidget)
  boost::optional<unsigned long> m_axesTag;
  QString m_widgetName;

  ViewBase *currentView; ///< Holder for the current (shown) view
  ViewBase *hiddenView;  ///< Holder for the view that is being switched from
  bool viewSwitched;

  pqLoadDataReaction *dataLoader; ///< Holder for the load data reaction
  double lodThreshold; ///< Default value for the LOD threshold (5 MB)
  QAction *lodAction;  ///< Holder for the LOD threshold menu item
  bool pluginMode;     ///< Flag to say widget is in plugin mode
  RotationPointDialog *rotPointDialog; ///< Holder for the rotation point dialog
  pqSaveScreenshotReaction *screenShot; ///< Holder for the screen shot reaction
  Ui::MdViewerWidgetClass ui;           ///< The MD viewer's UI form
  QHBoxLayout *viewLayout;              ///< Layout manager for the view widget
  pqApplicationSettingsReaction
      *viewSettings; ///< Holder for the view settings reaction
  bool useCurrentColorSettings;
  ModeControlWidget::Views initialView; ///< Holds the initial view
  MantidQt::API::MdSettings
      mdSettings; ///< Holds the MD settings which are used to persist data
  MantidQt::API::MdConstants mdConstants; /// < Holds the MD constants
  RebinAlgorithmDialogProvider m_rebinAlgorithmDialogProvider; ///< Provides
                                                               /// dialogs to
                                                               /// execute rebin
                                                               /// algorithms
  RebinnedSourcesManager
      m_rebinnedSourcesManager;          ///< Holds the rebinned sources manager
  QString m_rebinnedWorkspaceIdentifier; ///< Holds the identifier for temporary
                                         /// workspaces
  ColorMapEditorPanel
      *m_colorMapEditorPanel; ///< Holder for the color map editor panel.
  bool m_gridAxesStartUpOn;   /// flag for the initial grid axes setting
  Mantid::VATES::ColorScaleLock
      m_colorScaleLock; ///< Holds a color scale lock object

  /// Holds the 'visual state' of the views. This relies on Load/SaveXMLState
  /// which
  /// produce/consume a vtk XML tree object. Otherwise, the properties to save
  /// would be,
  /// at least, the following. vtkCamera: Position, FocalPpoint, ViewUp,
  /// ViewAngle,
  /// ClippingRange. pqRenderView: CenterOfRotation, CenterAxesVisibility
  struct AllVSIViewsState {
    AllVSIViewsState();
    ~AllVSIViewsState();
    void initialize();

    vtkSmartPointer<vtkPVXMLElement> stateStandard;
    vtkSmartPointer<vtkPVXMLElement> stateMulti;
    vtkSmartPointer<vtkPVXMLElement> stateThreeSlice;
    vtkSmartPointer<vtkPVXMLElement> stateSplatter;
  };
  AllVSIViewsState m_allViews;

  /// Setup color selection widget connections.
  void connectColorSelectionWidget();
  /// Setup connections for all dialogs.
  void connectDialogs();
  /// Setup rotation point dialog connections.
  void connectRotationPointDialog();
  /// Add view specific stuff to a menu.
  void createMenus();
  /// Disconnect dialog connections.
  void disconnectDialogs();
  /// Consolidate constructor related items.
  void internalSetup(bool pMode);
  /// Perform first render and final setup for mode buttons.
  void renderAndFinalSetup();
  /// Set the signals/slots for the ParaView components based on the view.
  void setParaViewComponentsForView();
  /// Run the necessary setup for the main view.
  void setupMainView(ModeControlWidget::Views viewType);
  /// Creates the UI and mode switch connection.
  void setupUiAndConnections();
  /// Create the requested view.
  ViewBase *createAndSetMainViewWidget(QWidget *container,
                                       ModeControlWidget::Views v,
                                       bool createRenderProxy = true);
  /// Helper function to swap current and hidden view pointers.
  void swapViews();
  /// Update the state of application widgets.
  void updateAppState();
  /// Get the initial view for the current workspace and user setting
  ModeControlWidget::Views getInitialView(int workspaceType,
                                          const std::string &instrumentName);
  /// Check that the view is valid for teh workspace type
  ModeControlWidget::Views
  checkViewAgainstWorkspace(ModeControlWidget::Views view, int workspaceType);
  /// Get the technique associated with an instrument.
  const std::string
  getTechniqueForInstrument(const std::string &instrumentName) const;
  /// Get the view for a specified instrument
  QString getViewForInstrument(const std::string &instrument) const;
  /// Check if a technique contains a keyword
  bool checkIfTechniqueContainsKeyword(const std::set<std::string> &techniques,
                                       const std::string &keyword) const;
  /// Reset the current view to the appropriate initial view.
  void resetCurrentView(int workspaceType, const std::string &instrumentName);
  /// Render rebinned workspace
  pqPipelineSource *
  prepareRebinnedWorkspace(const std::string &rebinnedWorkspaceName,
                           const std::string &sourceType);
  /// Handle drag and drop of peaks workspcaes
  void handleDragAndDropPeaksWorkspaces(QEvent *e, const QString &text,
                                        QStringList &wsNames);
  /// Set up the default color for the background of the view.
  void setColorForBackground();
  /// Sets axes colors that are visible against the background.
  void setVisibleAxesColors();
  /// Set the color map
  void setColorMap();
  /// Render the original workspace
  pqPipelineSource *
  renderOriginalWorkspace(const std::string &originalWorkspaceName);

  /// Remove the rebinning when switching views or otherwise.
  void
  removeRebinning(pqPipelineSource *source, bool forced,
                  ModeControlWidget::Views view = ModeControlWidget::STANDARD);
  /// Remove all rebinned sources
  void removeAllRebinning(ModeControlWidget::Views view);
  /// Sets a listener for when sources are being destroyed
  void setDestroyedListener();

  /// Save the state of the currently shown view so its state can be restored
  /// when switching back to it
  void saveViewState(ViewBase *view);
  /// Restore the state of the next (new) view when switching to it
  void restoreViewState(ViewBase *view, ModeControlWidget::Views vtype);
  /// Get the current grid axes setting
  bool areGridAxesOn();
  /// Load the state of VSI from an XML file
  bool loadVSIState(const std::string &fileName);
  /// Setup the view using the last active view and source from a project
  void setupViewFromProject(ModeControlWidget::Views vtype);
  /// Set the active objects on the current server
  void setActiveObjects(pqView *view, pqPipelineSource *source);
};

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // MDVIEWERWIDGET_H_