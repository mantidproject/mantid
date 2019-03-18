// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTWIDGET_H_
#define INSTRUMENTWIDGET_H_

#include "DllOption.h"
#include "InstrumentWidgetTypes.h"
#include "MantidGLWidget.h"
#include "UnwrappedSurface.h"

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidQtWidgets/Common/GraphOptions.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"

#include <boost/shared_ptr.hpp>

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

namespace MantidQt {
namespace MantidWidgets {
class InstrumentActor;
class InstrumentWidgetTab;
class InstrumentWidgetRenderTab;
class InstrumentWidgetMaskTab;
class InstrumentWidgetPickTab;
class InstrumentWidgetTreeTab;
class CollapsiblePanel;
class XIntegrationControl;
class SimpleWidget;
class ProjectionSurface;

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
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidget
    : public QWidget,
      public MantidQt::API::WorkspaceObserver,
      public Mantid::API::AlgorithmObserver,
      public InstrumentWidgetTypes {
  Q_OBJECT

  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;

public:
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

  explicit InstrumentWidget(const QString &wsName, QWidget *parent = nullptr,
                            bool resetGeometry = true, bool autoscaling = true,
                            double scaleMin = 0.0, double scaleMax = 0.0,
                            bool setDefaultView = true);
  ~InstrumentWidget() override;
  QString getWorkspaceName() const;
  std::string getWorkspaceNameStdString() const;
  void renameWorkspace(const std::string &workspace);
  SurfaceType getSurfaceType() const { return m_surfaceType; }
  Mantid::Kernel::V3D getSurfaceAxis(const int surfaceType) const;
  /// Get pointer to the projection surface
  boost::shared_ptr<ProjectionSurface> getSurface() const;
  /// True if the GL instrument display is currently on
  bool isGLEnabled() const;
  /// Toggle between the GL and simple instrument display widgets
  void enableOpenGL(bool on);
  /// Redraw the instrument view
  void updateInstrumentView(bool picking = true);
  /// Recalculate the detector data and redraw the instrument view
  void updateInstrumentDetectors();
  /// Delete the peaks workspace.
  void deletePeaksWorkspace(Mantid::API::IPeaksWorkspace_sptr pws);

  /// Alter data from a script. These just foward calls to the 3D widget
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setColorMapRange(double minValue, double maxValue);
  void selectComponent(const QString &name);
  void setScaleType(ColorMap::ScaleType type);
  void setExponent(double nth_power);
  void setViewType(const QString &type);
  const InstrumentActor &getInstrumentActor() const {
    return *m_instrumentActor;
  }
  InstrumentActor &getInstrumentActor() { return *m_instrumentActor; }
  void resetInstrument(bool resetGeometry);
  void resetSurface();
  void selectTab(int tab);
  void selectTab(Tab tab) { selectTab(int(tab)); }
  InstrumentWidgetTab *getTab(const QString &title = "") const;
  InstrumentWidgetTab *getTab(const Tab tab) const;
  /// Get a filename for saving
  QString getSaveFileName(const QString &title, const QString &filters,
                          QString *selectedFilter = nullptr);
  /// Get a name for settings group
  QString getSettingsGroupName() const;
  /// Get a name for a instrument-specific settings group
  QString getInstrumentSettingsGroupName() const;

  bool hasWorkspace(const std::string &wsName) const;
  void handleWorkspaceReplacement(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> workspace);

  /// Get the currently selected tab index
  int getCurrentTab() const;
  /// Load the widget from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the widget to a Mantid projecy file.
  std::string saveToProject() const;

signals:
  void enableLighting(bool);
  void plot1D(const QString &, const std::set<int> &, bool);
  void createDetectorTable(const QString &, const std::vector<int> &, bool);
  void needSetIntegrationRange(double, double);
  void surfaceTypeChanged(int);
  void colorMapChanged();
  void colorMapMinValueChanged(double);
  void colorMapMaxValueChanged(double);
  void colorMapRangeChanged(double, double);
  void scaleTypeChanged(int);
  void nthPowerChanged(double);
  void integrationRangeChanged(double, double);
  void glOptionChanged(bool);
  void requestSelectComponent(const QString &);
  void preDeletingHandle();
  void clearingHandle();
  void maskedWorkspaceOverlayed();

protected:
  /// Implements AlgorithmObserver's finish handler
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  void dragEnterEvent(QDragEnterEvent *e) override;
  void dropEvent(QDropEvent *e) override;
  bool eventFilter(QObject *obj, QEvent *ev) override;

public slots:
  void tabChanged(int);
  void componentSelected(size_t componentIndex);
  void executeAlgorithm(const QString &, const QString &);
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr);

  void setupColorMap();

  void changeColormap(const QString &cmapNameOrPath = ""); // Deprecated
  void changeScaleType(int);                               // Deprecated
  void changeNthPower(double);
  void changeColorMapMinValue(double minValue);               // Deprecated
  void changeColorMapMaxValue(double maxValue);               // Deprecated
  void changeColorMapRange(double minValue, double maxValue); // Deprecated
  void setIntegrationRange(double, double);
  void setBinRange(double, double);
  void disableColorMapAutoscaling(); // Deprecated
  void setColorMapAutoscaling(bool); // Deprecated

  void setViewDirection(const QString &);
  void pickBackgroundColor();
  void saveImage(QString filename);
  void setInfoText(const QString &);
  void set3DAxesState(bool);
  void setSurfaceType(int);
  void setWireframe(bool);

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
  void updateInfoText();

private slots:
  void helpClicked();

protected:
  void init(bool resetGeometry, bool autoscaling, double scaleMin,
            double scaleMax, bool setDefaultView, bool resetActor = true);
  /// Set newly created projection surface
  void setSurface(ProjectionSurface *surface);
  QWidget *createInstrumentTreeTab(QTabWidget *ControlsTab);
  void createTabs(QSettings &settings);
  void saveSettings();

  QString asString(const std::vector<int> &numbers) const;
  QString confirmDetectorOperation(const QString &opName,
                                   const QString &inputWS, int ndets);
  /// Set background color of the instrument display
  void setBackgroundColor(const QColor &color);
  /// Get the surface info string
  QString getSurfaceInfoText() const;
  /// Return the width of the instrunemt display
  int getInstrumentDisplayWidth() const;
  /// Return the height of the instrunemt display
  int getInstrumentDisplayHeight() const;
  /// Select the OpenGL or simple widget for instrument display
  void selectOpenGLDisplay(bool yes);
  /// Set the surface type.
  void setSurfaceType(const QString &typeStr);
  /// Return a filename to save a grouping to
  QString getSaveGroupingFilename();

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
  /// The OpenGL widget to display the instrument
  MantidGLWidget *m_InstrumentDisplay;
  /// The simple widget to display the instrument
  SimpleWidget *m_simpleDisplay;

  // Context menu actions
  QAction *m_clearPeakOverlays, *m_clearAlignment;

  /// The name of workspace that this window is associated with. The
  /// InstrumentActor holds a pointer to the workspace itself.
  QString m_workspaceName;
  /// Instrument actor is an interface to the instrument
  std::unique_ptr<InstrumentActor> m_instrumentActor;
  /// Option to use or not OpenGL display for "unwrapped" view, 3D is always in
  /// OpenGL
  bool m_useOpenGL;
  /// 3D view or unwrapped
  SurfaceType m_surfaceType;
  /// Stacked layout managing m_InstrumentDisplay and m_simpleDisplay
  QStackedLayout *m_instrumentDisplayLayout;
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

private:
  /// ADS notification handlers
  void preDeleteHandle(
      const std::string &ws_name,
      const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr) override;
  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr) override;
  void renameHandle(const std::string &oldName,
                    const std::string &newName) override;
  void clearADSHandle() override;
  /// overlay a peaks workspace on the projection surface
  void overlayPeaksWorkspace(Mantid::API::IPeaksWorkspace_sptr ws);
  /// overlay a masked workspace on the projection surface
  void overlayMaskedWorkspace(Mantid::API::IMaskWorkspace_sptr ws);
  /// overlay a table workspace with shape parameters on the projection surface
  void overlayShapesWorkspace(Mantid::API::ITableWorkspace_sptr);
  /// get a workspace from the ADS
  Mantid::API::Workspace_sptr getWorkspaceFromADS(const std::string &name);
  /// get a handle to the unwrapped surface
  boost::shared_ptr<UnwrappedSurface> getUnwrappedSurface();
  /// Load tabs on the widget form a project file
  void loadTabs(const std::string &lines) const;
  /// Save tabs on the widget to a string
  std::string saveTabs() const;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INSTRUMENTWIDGET_H_*/
