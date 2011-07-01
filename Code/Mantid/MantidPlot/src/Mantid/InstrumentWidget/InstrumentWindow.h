#ifndef INSTRUMENTWINDOW_H_
#define INSTRUMENTWINDOW_H_

//#include "Instrument3DWidget.h"
#include "MantidGLWidget.h"
#include "InstrumentTreeWidget.h"
#include "../../MdiSubWindow.h"
#include "../../GraphOptions.h"
#include "BinDialog.h"
#include "MantidQtAPI/WorkspaceObserver.h"

#include <string>
#include <vector>

#include "qwt_scale_widget.h"
#include <Poco/NObserver.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{

namespace API
{
class MatrixWorkspace;
}

}

class InstrumentActor;
class OneCurvePlot;
class CollapsiblePanel;
class InstrumentWindowRenderTab;
class InstrumentWindowPickTab;
class InstrumentWindowMaskTab;
class XIntegrationControl;

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

  InstrumentWindow(const QString& label = QString(), ApplicationWindow *app = 0, const QString& name = QString(), Qt::WFlags f = 0);
  ~InstrumentWindow();
  void setWorkspaceName(std::string wsName);
  QString getWorkspaceName()const{return QString::fromStdString(mWorkspaceName);}
  void updateWindow();

  SurfaceType getSurfaceType()const{return m_surfaceType;}

  /// Alter data from a script. These just foward calls to the 3D widget
  void setColorMapMinValue(double minValue);
  void setColorMapMaxValue(double maxValue);
  void setColorMapRange(double minValue, double maxValue);
  void selectComponent(const QString & name);
  void setScaleType(GraphOptions::ScaleType type);
  /// for saving the instrument window  to mantid project
  QString saveToString(const QString& geometry, bool saveAsTemplate= false);
  MantidGLWidget* getInstrumentDisplay(){return m_InstrumentDisplay;}
  InstrumentActor* getInstrumentActor(){return m_instrumentActor;}
  bool blocked()const{return m_blocked;}
  void selectTab(int tab);
//  const MantidColorMap & getColorMap() const{return m_instrumentActor->getColorMap();}

protected:
  /// Called just before a show event
  virtual void showEvent(QShowEvent* event);
  /// Implements AlgorithmObserver's finish handler
  void finishHandle(const Mantid::API::IAlgorithm* alg);

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

  //void componentSelected(const QItemSelection&, const QItemSelection&);
  void setViewDirection(const QString&);
  void pickBackgroundColor();
  void saveImage();
  void setInfoText(const QString&);
  void set3DAxesState(bool);
  void setSurfaceType(int);
  void setWireframe(bool);

signals:
  void plotSpectra(const QString&,const std::set<int>&);
  void createDetectorTable(const QString&,const std::vector<int>&,bool);
  void execMantidAlgorithm(const QString&,const QString&,Mantid::API::AlgorithmObserver*);
  void needSetIntegrationRange(double,double);

private slots:
  void block();
  void unblock();

private:

  //QFrame * createPickTab(QTabWidget* ControlsTab);
  QFrame * createInstrumentTreeTab(QTabWidget* ControlsTab);

  void loadSettings();
  void saveSettings();

  QString asString(const std::vector<int>& numbers) const;
  QString confirmDetectorOperation(const QString & opName, const QString & inputWS, int ndets);
  QLabel*      mInteractionInfo;
  QTabWidget*  mControlsTab;
  // Actions for the pick menu
  QAction *mInfoAction, *mPlotAction, *mDetTableAction, *mGroupDetsAction, *mMaskDetsAction;
  QAction *m_ExtractDetsToWorkspaceAction;  ///< Extract selected detector ids to a new workspace
  QAction *m_SumDetsToWorkspaceAction;      ///< Sum selected detectors to a new workspace
  QAction *m_createIncludeGroupingFileAction; ///< Create grouping xml file which includes selected detectors
  QAction *m_createExcludeGroupingFileAction; ///< Create grouping xml file which excludes selected detectors

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  MantidGLWidget* m_InstrumentDisplay;
  InstrumentActor* m_instrumentActor;
  SurfaceType m_surfaceType;       ///< 3D view or unwrapped

  int          mSpectraIDSelected; ///< spectra index id
  int          mDetectorIDSelected; ///< detector id
  std::set<int> mSpectraIDSelectedList;
  std::vector<int> mDetectorIDSelectedList;
  InstrumentTreeWidget* mInstrumentTree; ///< Widget to display instrument tree

  std::string mWorkspaceName; ///< The name of workpace that this window is associated with
  QString mDefaultColorMap; ///< The full path of the default color map
  QString m_savedialog_dir; /// The last used dialog directory

  InstrumentWindowRenderTab * m_renderTab;
  InstrumentWindowPickTab * m_pickTab;
  InstrumentWindowMaskTab * m_maskTab;
  XIntegrationControl * m_xIntegration;

  bool mViewChanged;                ///< stores whether the user changed the view (so don't automatically change it)

  bool m_blocked;     ///< Set to true to block access to instrument during algorithm executions
  QList<int> m_selectedDetectors;


private:
  virtual void deleteHandle(const std::string & ws_name, const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> workspace_ptr);
  virtual void clearADSHandle();
};

#endif /*INSTRUMENTWINDOW_H_*/

