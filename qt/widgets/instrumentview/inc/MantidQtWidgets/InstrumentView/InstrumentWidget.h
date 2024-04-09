// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IGLDisplay.h"
#include "IQtDisplay.h"
#include "InstrumentDisplay.h"
#include "InstrumentWidgetTypes.h"
#include "QtConnect.h"
#include "QtMetaObject.h"
#include "UnwrappedSurface.h"

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidQtWidgets/Common/GraphOptions.h"
#include "MantidQtWidgets/Common/IMessageHandler.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"

#include <memory>

#include <QThread>

namespace Mantid {
namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid

// Qt forward declarations
class QPushButton;
class QDialog;
class QSlider;
class QTabWidget;
class QLineEdit;
class QLabel;
class QCheckBox;
class QComboBox;
class QTextEdit;
class QShowEvent;
class QDragEnterEvent;
class QDropEvent;
class QStackedLayout;
class QSettings;
class QSplitter;
class QVBoxLayout;

namespace MantidQt {
namespace MantidWidgets {
class InstrumentActor;
class InstrumentWidgetTab;
class InstrumentWidgetRenderTab;
class InstrumentWidgetMaskTab;
class InstrumentWidgetTreeTab;
class CollapsiblePanel;
class XIntegrationControl;
class QtDisplay;
class ProjectionSurface;

namespace Detail {
struct Dependencies {
  std::unique_ptr<IInstrumentDisplay> instrumentDisplay = nullptr;
  std::unique_ptr<IQtDisplay> qtDisplay = nullptr;
  std::unique_ptr<IGLDisplay> glDisplay = nullptr;
  std::unique_ptr<QtConnect> qtConnect = std::make_unique<QtConnect>();
  std::unique_ptr<QtMetaObject> qtMetaObject = std::make_unique<QtMetaObject>();
  std::unique_ptr<IMessageHandler> messageHandler = nullptr;
};

struct TabCustomizations {
  std::vector<IWPickToolType> pickTools = std::vector<IWPickToolType>{
      IWPickToolType::Zoom,          IWPickToolType::PixelSelect,     IWPickToolType::WholeInstrumentSelect,
      IWPickToolType::TubeSelect,    IWPickToolType::PeakSelect,      IWPickToolType::PeakErase,
      IWPickToolType::PeakCompare,   IWPickToolType::PeakAlign,       IWPickToolType::DrawEllipse,
      IWPickToolType::DrawRectangle, IWPickToolType::DrawSector,      IWPickToolType::DrawFree,
      IWPickToolType::EditShape,     IWPickToolType::DrawRingEllipse, IWPickToolType::DrawRingRectangle};
};

} // namespace Detail

/**
\class  InstrumentWidget
\brief  This is the main window for the control of display on geometry
\author Srikanth Nagella
\date   September 2008
\version 1.0

This is a QT widget for the controls and display of instrument geometry.
The user documentation can be found at
http://www.mantidproject.org/MantidPlot:_Instrument_View
and needs to be updated whenever the instrument view functionality changes.

*/
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidget : public QWidget,
                                                            public MantidQt::API::WorkspaceObserver,
                                                            public Mantid::API::AlgorithmObserver,
                                                            public InstrumentWidgetTypes {
  Q_OBJECT

  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;

public:
  using Dependencies = Detail::Dependencies;
  using TabCustomizations = Detail::TabCustomizations;
  enum SurfaceType {
    FULL3D = 0,
    CYLINDRICAL_X,
    CYLINDRICAL_Y,
    CYLINDRICAL_Z,
    SPHERICAL_X,
    SPHERICAL_Y,
    SPHERICAL_Z,
    SIDE_BY_SIDE,
    RENDERMODE_SIZE
  };
  enum Tab { RENDER = 0, PICK, MASK, TREE };

  explicit InstrumentWidget(QString wsName, QWidget *parent = nullptr, bool resetGeometry = true,
                            bool autoscaling = true, double scaleMin = 0.0, double scaleMax = 0.0,
                            bool setDefaultView = true, Dependencies deps = Dependencies(), bool useThread = false,
                            QString settingsGroup = "Mantid/InstrumentWidget",
                            TabCustomizations customizations = TabCustomizations());
  ~InstrumentWidget() override;
  QString getWorkspaceName() const;
  std::string getWorkspaceNameStdString() const;
  Mantid::API::Workspace_sptr getWorkspaceClone();
  void renameWorkspace(const std::string &workspace);
  SurfaceType getSurfaceType() const { return m_surfaceType; }
  Mantid::Kernel::V3D getSurfaceAxis(const int surfaceType) const;
  /// Get pointer to the projection surface
  std::shared_ptr<ProjectionSurface> getSurface() const;
  /// True if the workspace is being replaced
  bool isWsBeingReplaced() const;
  /// True if the GL instrument display is currently on
  bool isGLEnabled() const;
  /// Toggle between the GL and simple instrument display widgets
  void enableOpenGL(bool on);
  /// Redraw the instrument view
  void updateInstrumentView(bool picking = true);
  /// Recalculate the detector data and redraw the instrument view
  void updateInstrumentDetectors();
  /// Delete the peaks workspace.
  void deletePeaksWorkspace(const Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Alter data from a script. These just foward calls to the 3D widget
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setColorMapRange(double minValue, double maxValue);
  void selectComponent(const QString &name);
  void setScaleType(ColorMap::ScaleType type);
  void setExponent(double nth_power);
  void setViewType(const QString &type);
  const InstrumentActor &getInstrumentActor() const { return *m_instrumentActor; }
  InstrumentActor &getInstrumentActor() { return *m_instrumentActor; }
  void resetInstrument(bool resetGeometry);
  void resetSurface();
  void resetInstrumentActor(bool resetGeometry, bool autoscaling, double scaleMin, double scaleMax,
                            bool setDefaultView);
  void selectTab(int tab);
  void selectTab(Tab tab) { selectTab(int(tab)); }
  InstrumentWidgetTab *getTab(const QString &title = "") const;
  InstrumentWidgetTab *getTab(const Tab tab) const;
  /// Get a specific tab
  InstrumentWidgetRenderTab *getRenderTab(const Tab tab) const;
  /// Get a Pick tab
  InstrumentWidgetPickTab *getPickTab(const Tab tab) const;

  /// Get a filename for saving
  QString getSaveFileName(const QString &title, const QString &filters, QString *selectedFilter = nullptr);
  /// Get a name for settings group
  inline QString getSettingsGroupName() const noexcept { return m_settingsGroup; }
  /// Get a name for a instrument-specific settings group
  QString getInstrumentSettingsGroupName() const;

  bool hasWorkspace(const std::string &wsName) const;
  void handleWorkspaceReplacement(const std::string &wsName, const std::shared_ptr<Mantid::API::Workspace> &workspace);
  void replaceWorkspace(const std::string &newWs, const std::string &newInstrumentWindowName);

  /// Get the currently selected tab index
  int getCurrentTab() const;
  /// Decides whether the given tab is the tab currently open
  bool isCurrentTab(InstrumentWidgetTab *tab) const;
  /// Load the widget from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the widget to a Mantid projecy file.
  std::string saveToProject() const;
  void removeTab(const std::string &tabName);
  void addTab(const std::string &tabName);
  void hideHelp();
  InstrumentWidgetPickTab *getPickTab() { return m_pickTab; }
  /// Determine if the workspace requires an integration bar
  bool isIntegrable();
  /// Whether the background instrument actor loading thead is running
  bool isThreadRunning() const;
  /// Block until thread is finished
  void waitForThread() const;
  /// Whether the window has been fully initialized
  bool isFinished() const;

  /// Whether the side tab is currently visible or is folded
  bool isTabFolded() const;

  IInstrumentDisplay *getInstrumentDisplay() const { return m_instrumentDisplay.get(); };

signals:
  void enableLighting(bool /*_t1*/);
  void plot1D(const QString & /*_t1*/, const std::set<int> & /*_t2*/, bool /*_t3*/);
  void createDetectorTable(const QString & /*_t1*/, const std::vector<int> & /*_t2*/, bool /*_t3*/);
  void needSetIntegrationRange(double /*_t1*/, double /*_t2*/);
  void surfaceTypeChanged(int /*_t1*/);
  void maintainAspectRatioChanged(bool /*on*/);
  void colorMapChanged();
  void colorMapMinValueChanged(double /*_t1*/);
  void colorMapMaxValueChanged(double /*_t1*/);
  void colorMapRangeChanged(double /*_t1*/, double /*_t2*/);
  void scaleTypeChanged(int /*_t1*/);
  void nthPowerChanged(double /*_t1*/);
  void integrationRangeChanged(double /*_t1*/, double /*_t2*/);
  void glOptionChanged(bool /*_t1*/);
  void requestSelectComponent(const QString & /*_t1*/);
  void preDeletingHandle();
  void clearingHandle();
  void maskedWorkspaceOverlayed();
  void instrumentActorReset();

protected:
  /// Implements AlgorithmObserver's finish handler
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  void dragEnterEvent(QDragEnterEvent *e) override;
  void dropEvent(QDropEvent *e) override;
  bool eventFilter(QObject *obj, QEvent *ev) override;
  void closeEvent(QCloseEvent *e) override;

public slots:
  void tabChanged(int /*unused*/);
  void componentSelected(size_t componentIndex);
  void executeAlgorithm(const QString & /*unused*/, const QString & /*unused*/);
  void executeAlgorithm(const Mantid::API::IAlgorithm_sptr & /*alg*/);

  void setupColorMap();

  void changeColormap(const QString &cmapNameOrPath = "", const bool highlightZeroDets = false); // Deprecated
  void changeScaleType(int /*type*/);                                                            // Deprecated
  void changeNthPower(double /*nth_power*/);
  void changeColorMapMinValue(double minValue);               // Deprecated
  void changeColorMapMaxValue(double maxValue);               // Deprecated
  void changeColorMapRange(double minValue, double maxValue); // Deprecated
  void setIntegrationRange(double /*xmin*/, double /*xmax*/);
  void setBinRange(double /*xmin*/, double /*xmax*/);
  void disableColorMapAutoscaling();        // Deprecated
  void setColorMapAutoscaling(bool /*on*/); // Deprecated

  void setViewDirection(const QString & /*input*/);
  void pickBackgroundColor();
  void freezeRotation(bool);
  void saveImage(QString filename);
  void setInfoText(const QString & /*text*/);
  void set3DAxesState(bool /*on*/);
  void setSurfaceType(int /*type*/);
  void setWireframe(bool /*on*/);
  void setMaintainAspectRatio(bool /*on*/);

  /// Overlay a workspace with the given name
  bool overlay(const QString &wsName);
  void clearPeakOverlays();
  void clearAlignmentPlane();
  void setPeakLabelPrecision(int n);
  void setShowPeakRowFlag(bool on);
  void setShowPeakLabelsFlag(bool on);
  void setShowPeakRelativeIntensity(bool on);

  /// Enable OpenGL. Slot called from render tab only - doesn't update the
  /// checkbox.
  void enableGL(bool on);
  void updateInfoText(const QString &text = QString());

  void initWidget(bool resetGeometry, bool setDefaultView);
  void threadFinished();

private slots:
  void helpClicked();

protected:
  void init(bool resetGeometry, bool setDefaultView);
  /// Set newly created projection surface
  void setSurface(ProjectionSurface *surface);
  QWidget *createInstrumentTreeTab(QTabWidget *ControlsTab);
  void createTabs(const QSettings &settings, TabCustomizations customizations);
  void saveSettings();

  QString asString(const std::vector<int> &numbers) const;
  QString confirmDetectorOperation(const QString &opName, const QString &inputWS, int ndets);
  /// Set background color of the instrument display
  void setBackgroundColor(const QColor &color);
  /// Get the surface info string
  QString getSurfaceInfoText() const;
  /// Select the OpenGL or simple widget for instrument display
  void selectOpenGLDisplay(bool yes);
  /// Set the surface type.
  void setSurfaceType(const QString &typeStr);
  /// Return a filename to save a grouping to
  QString getSaveGroupingFilename();
  /// add the selected tabs
  void addSelectedTabs();
  /// update integration widget visibility and range
  void updateIntegrationWidget(bool init = false);

  // GUI elements
  QLabel *mInteractionInfo;

  QTabWidget *mControlsTab;
  /// Control tabs
  QList<InstrumentWidgetTab *> m_tabs;
  InstrumentWidgetRenderTab *m_renderTab;
  InstrumentWidgetMaskTab *m_maskTab;
  InstrumentWidgetTreeTab *m_treeTab;
  InstrumentWidgetPickTab *m_pickTab;
  XIntegrationControl *m_xIntegration;

  std::unique_ptr<IInstrumentDisplay> m_instrumentDisplay;

  // Context menu actions
  QAction *m_clearPeakOverlays, *m_clearAlignment;

  /// The name of workspace that this window is associated with. The
  /// InstrumentActor holds a pointer to the workspace itself.
  QString m_workspaceName;
  /// The name of the settings group to store settings in
  QString m_settingsGroup;
  /// Instrument actor is an interface to the instrument
  std::unique_ptr<InstrumentActor> m_instrumentActor;
  /// Option to use or not OpenGL display for "unwrapped" view, 3D is always in
  /// OpenGL
  bool m_useOpenGL;
  /// 3D view or unwrapped
  SurfaceType m_surfaceType;

  /// spectra index id
  int mSpectraIDSelected;
  /// detector id
  int mDetectorIDSelected;
  std::set<int> mSpectraIDSelectedList;
  std::vector<int> mDetectorIDSelectedList;

  /// The full path of the default color map
  QString mDefaultColorMap;
  /// The last used dialog directory
  QString m_savedialog_dir;

  /// stores whether the user changed the view (so don't automatically change
  /// it)
  bool mViewChanged;
  /// Set to true to block access to instrument during algorithm executions
  bool m_blocked;
  QList<int> m_selectedDetectors;
  bool m_instrumentDisplayContextMenuOn;
  /// dict of selected tabs
  std::vector<std::pair<std::string, bool>> m_stateOfTabs;

private:
  /// ADS notification handlers
  void preDeleteHandle(const std::string &ws_name,
                       const std::shared_ptr<Mantid::API::Workspace> &workspace_ptr) override;
  void afterReplaceHandle(const std::string &wsName,
                          const std::shared_ptr<Mantid::API::Workspace> &workspace_ptr) override;
  void renameHandle(const std::string &oldName, const std::string &newName) override;
  void clearADSHandle() override;
  /// close the widget after an ADS event removes the workspace
  virtual void handleActiveWorkspaceDeleted();
  /// overlay a peaks workspace on the projection surface
  void overlayPeaksWorkspace(const Mantid::API::IPeaksWorkspace_sptr &ws);
  /// overlay a masked workspace on the projection surface
  void overlayMaskedWorkspace(const Mantid::API::IMaskWorkspace_sptr &ws);
  /// overlay a table workspace with shape parameters on the projection surface
  void overlayShapesWorkspace(const Mantid::API::ITableWorkspace_sptr & /*ws*/);
  /// get a workspace from the ADS
  Mantid::API::Workspace_sptr getWorkspaceFromADS(const std::string &name);
  /// get a handle to the unwrapped surface
  std::shared_ptr<UnwrappedSurface> getUnwrappedSurface();
  /// Load tabs on the widget form a project file
  void loadTabs(const std::string &lines) const;
  /// Save tabs on the widget to a string
  std::string saveTabs() const;

  void cancelThread();

  bool m_wsReplace;
  QPushButton *m_help;
  QVBoxLayout *m_mainLayout;

  /// Wrapper around Qt connect function so we can mock it
  std::unique_ptr<QtConnect> m_qtConnect;
  /// Wrapper around Qt meta object for mocking
  std::unique_ptr<QtMetaObject> m_qtMetaObject;

  std::unique_ptr<IMessageHandler> m_messageHandler;

  mutable QThread m_thread;
  bool m_finished;
  bool m_autoscaling;
  double m_scaleMin;
  double m_scaleMax;
  bool m_setDefaultView;
  bool m_resetGeometry;
  bool m_useThread;
  bool m_maintainAspectRatio;

  QSplitter *m_controlPanelLayout;
};

} // namespace MantidWidgets
} // namespace MantidQt
