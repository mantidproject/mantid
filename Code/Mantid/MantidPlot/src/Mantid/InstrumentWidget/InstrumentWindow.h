#ifndef INSTRUMENTWINDOW_H_
#define INSTRUMENTWINDOW_H_

#include "../../MdiSubWindow.h"
#include "../MantidAlgorithmMetatype.h"

#include "MantidGLWidget.h"
#include "BinDialog.h"

#include "MantidQtAPI/GraphOptions.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "Mantid/IProjectSerialisable.h"

#include <string>
#include <vector>

#include "qwt_scale_widget.h"
#include <Poco/NObserver.h>
#include <boost/shared_ptr.hpp>

class InstrumentActor;
class OneCurvePlot;
class CollapsiblePanel;
class InstrumentWindowTab;
class XIntegrationControl;
class SimpleWidget;
class ProjectionSurface;
class InstrumentWindowRenderTab;

// Qt forward declarations
class QPushButton;
class QDialog;
class QSlider;
class QSpinBox;
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

/**
  \class  InstrumentWindow
  \brief  This is the main window for the control of display on geometry
  \author Srikanth Nagella
  \date   September 2008
  \version 1.0

  This is a QT widget for the controls and display of instrument geometry. 
  The user documentation can be found at http://www.mantidproject.org/MantidPlot:_Instrument_View
  and needs to be updated whenever the instrument view functionality changes.

 */
class InstrumentWindow : public MdiSubWindow, public MantidQt::API::WorkspaceObserver, public Mantid::API::AlgorithmObserver, public Mantid::IProjectSerialisable
{
  Q_OBJECT

public:
  enum SurfaceType{ FULL3D = 0, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z, SIDE_BY_SIDE, RENDERMODE_SIZE };
  enum Tab{RENDER = 0, PICK, MASK, TREE};

  explicit InstrumentWindow(const QString& wsName, const QString& label = QString(), ApplicationWindow *app = 0, const QString& name = QString(), Qt::WFlags f = 0);
  ~InstrumentWindow();
  void init(bool resetGeometry = true, bool autoscaling = true, double scaleMin = 0.0, double scaleMax = 0.0, bool setDefaultView = true);
  QString getWorkspaceName() const { return m_workspaceName; }

  SurfaceType getSurfaceType()const{return m_surfaceType;}
  /// Get pointer to the projection surface
  boost::shared_ptr<ProjectionSurface> getSurface() const;
  /// True if the GL instrument display is currently on
  bool isGLEnabled() const;
  /// Toggle between the GL and simple instrument display widgets
  void enableOpenGL( bool on );
  /// Redraw the instrument view
  void updateInstrumentView(bool picking = true);
  /// Recalculate the detector data and redraw the instrument view
  void updateInstrumentDetectors();

  /// Alter data from a script. These just foward calls to the 3D widget
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setColorMapRange(double minValue, double maxValue);
  void selectComponent(const QString & name);
  void setScaleType(GraphOptions::ScaleType type);
  void setViewType(const QString& type);
  InstrumentActor* getInstrumentActor() const {return m_instrumentActor;}
  bool blocked()const{return m_blocked;}
  void selectTab(int tab);
  void selectTab(Tab tab){selectTab(int(tab));}
  InstrumentWindowTab *getTab(const QString & title="") const;
  InstrumentWindowTab *getTab(const Tab tab) const;
  /// Get a filename for saving
  QString getSaveFileName(const QString& title, const QString& filters, QString* selectedFilter = NULL);
  /// Get a name for settings group
  QString getSettingsGroupName() const;
  /// Get a name for a instrument-specific settings group
  QString getInstrumentSettingsGroupName() const;

  void loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion);
  std::string saveToProject(ApplicationWindow* app);

signals:
  void enableLighting(bool);
  void plot1D(const QString&,const std::set<int>&,bool);
  void createDetectorTable(const QString&,const std::vector<int>&,bool);
  void execMantidAlgorithm(const QString&,const QString&,Mantid::API::AlgorithmObserver*);
  void execMantidAlgorithm(Mantid::API::IAlgorithm_sptr);
  void needSetIntegrationRange(double,double);
  void surfaceTypeChanged(int);
  void colorMapChanged();
  void colorMapMinValueChanged(double);
  void colorMapMaxValueChanged(double);
  void colorMapRangeChanged(double,double);
  void scaleTypeChanged(int);
  void integrationRangeChanged(double,double);
  void glOptionChanged(bool);
  void requestSelectComponent(const QString&);

protected:
  /// Called just before a show event
  virtual void showEvent(QShowEvent* event);
  /// Implements AlgorithmObserver's finish handler
  void finishHandle(const Mantid::API::IAlgorithm* alg);
  void dragEnterEvent( QDragEnterEvent* e );
  void dropEvent( QDropEvent* e );
  bool eventFilter(QObject *obj, QEvent *ev);

public slots:
  void tabChanged(int);
  void componentSelected(Mantid::Geometry::ComponentID id);
  void executeAlgorithm(const QString&, const QString&);
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr);

  void setupColorMap();

  void changeColormap(const QString & filename = ""); // Deprecated
  void changeScaleType(int);// Deprecated
  void changeColorMapMinValue(double minValue); // Deprecated
  void changeColorMapMaxValue(double maxValue); // Deprecated
  void changeColorMapRange(double minValue, double maxValue); // Deprecated
  void setIntegrationRange(double,double);
  void setBinRange(double,double);
  void setColorMapAutoscaling(bool); // Deprecated

  void setViewDirection(const QString&);
  void pickBackgroundColor();
  void saveImage(QString filename);
  void setInfoText(const QString&);
  void set3DAxesState(bool);
  void setSurfaceType(int);
  void setWireframe(bool);

  /// Overlay a workspace with the given name
  bool overlay(const QString & wsName);
  void clearPeakOverlays();
  void setPeakLabelPrecision(int n);
  void setShowPeakRowFlag(bool on);
  void setShowPeakLabelsFlag(bool on);
  /// Enable OpenGL. Slot called from render tab only - doesn't update the checkbox.
  void enableGL( bool on );
  void updateInfoText();

private slots:
  void block();
  void unblock();
  void helpClicked();

private:
  /// Set newly created projection surface
  void setSurface(ProjectionSurface* surface);
  QWidget * createInstrumentTreeTab(QTabWidget* ControlsTab);
  void createTabs(QSettings& settings);
  void saveSettings();

  QString asString(const std::vector<int>& numbers) const;
  QString confirmDetectorOperation(const QString & opName, const QString & inputWS, int ndets);
  /// Set background color of the instrument display
  void setBackgroundColor(const QColor& color);
  /// Get the surface info string
  QString getSurfaceInfoText() const;
  /// Return the width of the instrunemt display
  int getInstrumentDisplayWidth() const;
  /// Return the height of the instrunemt display
  int getInstrumentDisplayHeight() const;
  /// Select the OpenGL or simple widget for instrument display
  void selectOpenGLDisplay(bool yes);
  /// Set the surface type.
  void setSurfaceType(const QString& typeStr);
  /// Return a filename to save a grouping to
  QString getSaveGroupingFilename();

  // GUI elements
  QLabel*      mInteractionInfo;
  QTabWidget*  mControlsTab;
  /// Control tabs
  QList<InstrumentWindowTab *> m_tabs;
  InstrumentWindowRenderTab *m_renderTab;
  XIntegrationControl * m_xIntegration;
  /// The OpenGL widget to display the instrument
  MantidGLWidget* m_InstrumentDisplay;
  /// The simple widget to display the instrument
  SimpleWidget* m_simpleDisplay;

  // Context menu actions
  QAction *m_clearPeakOverlays;

  /// The name of workspace that this window is associated with. The InstrumentActor holds a pointer to the workspace itself.
  QString m_workspaceName;
  /// Instrument actor is an interface to the instrument
  InstrumentActor* m_instrumentActor;
  /// Option to use or not OpenGL display for "unwrapped" view, 3D is always in OpenGL
  bool m_useOpenGL;
  /// 3D view or unwrapped
  SurfaceType m_surfaceType;       
  /// Stacked layout managing m_InstrumentDisplay and m_simpleDisplay
  QStackedLayout* m_instrumentDisplayLayout;
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

  /// stores whether the user changed the view (so don't automatically change it)
  bool mViewChanged;
  /// Set to true to block access to instrument during algorithm executions
  bool m_blocked;     
  QList<int> m_selectedDetectors;
  bool m_instrumentDisplayContextMenuOn;

private:
  /// ADS notification handlers
  virtual void preDeleteHandle(const std::string & ws_name, const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void renameHandle(const std::string &oldName, const std::string &newName);
  virtual void clearADSHandle();
};

#endif /*INSTRUMENTWINDOW_H_*/

