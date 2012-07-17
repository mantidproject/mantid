#ifndef INSTRUMENTWINDOW_H_
#define INSTRUMENTWINDOW_H_

#include "MantidGLWidget.h"
#include "InstrumentTreeWidget.h"
#include "../../MdiSubWindow.h"
#include "MantidQtAPI/GraphOptions.h"
#include "BinDialog.h"
#include "MantidQtAPI/WorkspaceObserver.h"

#include <string>
#include <vector>

#include "qwt_scale_widget.h"
#include <Poco/NObserver.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmObserver.h"

class InstrumentActor;
class OneCurvePlot;
class CollapsiblePanel;
class InstrumentWindowRenderTab;
class InstrumentWindowPickTab;
class InstrumentWindowMaskTab;
class XIntegrationControl;
class SimpleWidget;
class ProjectionSurface;

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

/**
  \class  InstrumentWindow
  \brief  This is the main window for the control of display on geometry
  \author Srikanth Nagella
  \date   September 2008
  \version 1.0

  This is a QT widget for the controls and display of instrument geometry

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
 */
class InstrumentWindow : public MdiSubWindow, public MantidQt::API::WorkspaceObserver, public Mantid::API::AlgorithmObserver
{
  Q_OBJECT

public:
  enum SurfaceType{ FULL3D = 0, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z, RENDERMODE_SIZE };
  enum Tab{RENDER = 0, PICK, MASK, TREE};

  explicit InstrumentWindow(const QString& wsName, const QString& label = QString(), ApplicationWindow *app = 0, const QString& name = QString(), Qt::WFlags f = 0);
  ~InstrumentWindow();
  void init();
  QString getWorkspaceName() const { return m_workspaceName; }
  void updateWindow();

  SurfaceType getSurfaceType()const{return m_surfaceType;}
  /// Get pointer to the projection surface
  ProjectionSurface* getSurface() const;
  /// Set newly created projection surface
  void setSurface(ProjectionSurface* surface);

  /// True if the GL instrument display is currently on
  bool isGLEnabled() const;

  /// Alter data from a script. These just foward calls to the 3D widget
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setColorMapRange(double minValue, double maxValue);
  void selectComponent(const QString & name);
  void setScaleType(GraphOptions::ScaleType type);
  void setViewType(const QString& type);
  /// for saving the instrument window  to mantid project
  QString saveToString(const QString& geometry, bool saveAsTemplate= false);
  MantidGLWidget* getInstrumentDisplay(){return m_InstrumentDisplay;}
  InstrumentActor* getInstrumentActor(){return m_instrumentActor;}
  bool blocked()const{return m_blocked;}
  void selectTab(int tab);
  void selectTab(Tab tab){selectTab(int(tab));}
  Tab getTab()const;

protected:
  /// Called just before a show event
  virtual void showEvent(QShowEvent* event);
  /// Implements AlgorithmObserver's finish handler
  void finishHandle(const Mantid::API::IAlgorithm* alg);
  void dragEnterEvent( QDragEnterEvent* e );
  void dropEvent( QDropEvent* e );
  bool eventFilter(QObject *obj, QEvent *ev);

public slots:
  void tabChanged(int i);
  void singleDetectorPicked(int);
  void singleDetectorTouched(int);
  void multipleDetectorsSelected(QList<int>&);
  void showPickOptions();
  void spectraInfoDialog();
  void plotSelectedSpectra();
  void showDetectorTable();
  void groupDetectors();
  void maskDetectors();
  void executeAlgorithm(const QString&, const QString&);

  void extractDetsToWorkspace();
  void sumDetsToWorkspace();
  void createIncludeGroupingFile();
  void createExcludeGroupingFile();

  void setupColorMap();
  void changeColormap(const QString & filename = "");
  void changeScaleType(int);
  void changeColorMapMinValue(double minValue);
  void changeColorMapMaxValue(double maxValue);
  void changeColorMapRange(double minValue, double maxValue);
  void setIntegrationRange(double,double);
  void setBinRange(double,double);
  void setColorMapAutoscaling(bool);

  void setViewDirection(const QString&);
  void pickBackgroundColor();
  void saveImage();
  void setInfoText(const QString&);
  void set3DAxesState(bool);
  void setSurfaceType(int);
  void setWireframe(bool);

  void clearPeakOverlays();
  void setPeakLabelPrecision(int n);
  void setShowPeakRowFlag(bool on);
  /// Toggle between the GL and simple instrument display widgets
  void enableGL( bool on );

signals:
  void plotSpectra(const QString&,const std::set<int>&);
  void createDetectorTable(const QString&,const std::vector<int>&,bool);
  void execMantidAlgorithm(const QString&,const QString&,Mantid::API::AlgorithmObserver*);
  void needSetIntegrationRange(double,double);

private slots:
  void block();
  void unblock();
  void mouseLeftInstrumentDisplay();

private:
  QWidget * createInstrumentTreeTab(QTabWidget* ControlsTab);
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
  /// Refresh the instrument display
  void refreshInstrumentDisplay();


  QLabel*      mInteractionInfo;
  QTabWidget*  mControlsTab;
  // Actions for the pick menu
  QAction *mInfoAction, *mPlotAction, *mDetTableAction, *mGroupDetsAction, *mMaskDetsAction;
  QAction *m_ExtractDetsToWorkspaceAction;  ///< Extract selected detector ids to a new workspace
  QAction *m_SumDetsToWorkspaceAction;      ///< Sum selected detectors to a new workspace
  QAction *m_createIncludeGroupingFileAction; ///< Create grouping xml file which includes selected detectors
  QAction *m_createExcludeGroupingFileAction; ///< Create grouping xml file which excludes selected detectors
  // Context menu actions
  QAction *m_clearPeakOverlays;

  /// The name of workspace that this window is associated with. The InstrumentActor holds a pointer to the workspace itself.
  const QString m_workspaceName;
  MantidGLWidget* m_InstrumentDisplay;
  SimpleWidget* m_simpleDisplay;
  InstrumentActor* m_instrumentActor;
  SurfaceType m_surfaceType;       ///< 3D view or unwrapped
  QStackedLayout* m_instrumentDisplayLayout;

  int          mSpectraIDSelected; ///< spectra index id
  int          mDetectorIDSelected; ///< detector id
  std::set<int> mSpectraIDSelectedList;
  std::vector<int> mDetectorIDSelectedList;
  InstrumentTreeWidget* mInstrumentTree; ///< Widget to display instrument tree

  QString mDefaultColorMap; ///< The full path of the default color map
  QString m_savedialog_dir; /// The last used dialog directory

  InstrumentWindowRenderTab * m_renderTab;
  InstrumentWindowPickTab * m_pickTab;
  InstrumentWindowMaskTab * m_maskTab;
  XIntegrationControl * m_xIntegration;

  bool mViewChanged;                ///< stores whether the user changed the view (so don't automatically change it)

  bool m_blocked;     ///< Set to true to block access to instrument during algorithm executions
  QList<int> m_selectedDetectors;
  bool m_instrumentDisplayContextMenuOn;

private:
  virtual void preDeleteHandle(const std::string & ws_name, const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void clearADSHandle();
};

#endif /*INSTRUMENTWINDOW_H_*/

