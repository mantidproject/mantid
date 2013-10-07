#include "InstrumentWindow.h"
#include "InstrumentWindowPickTab.h"
#include "OneCurvePlot.h"
#include "CollapsiblePanel.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"
#include "UnwrappedSurface.h"
#include "Projection3D.h"
#include "PeakMarker2D.h"

#include "MantidKernel/ConfigService.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "qwt_scale_widget.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QWidgetAction>
#include <QLabel>
#include <QMessageBox>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalMapper>
#include <QPixmap>
#include <QSettings>
#include <QApplication>

#include <numeric>
#include <cfloat>
#include <cmath>
#include <algorithm>

/// to be used in std::transform
struct Sqrt
{
  double operator()(double x)
  {
    return sqrt(x);
  }
};

/**
 * Constructor.
 * @param instrWindow :: Parent InstrumentWindow.
 */
InstrumentWindowPickTab::InstrumentWindowPickTab(InstrumentWindow* instrWindow):
InstrumentWindowTab(instrWindow),
m_currentDetID(-1),
m_tubeXUnits(DETECTOR_ID),
m_freezePlot(false)
{

  // connect to InstrumentWindow signals
  connect(m_instrWindow,SIGNAL(integrationRangeChanged(double,double)),this,SLOT(changedIntegrationRange(double,double)));

  m_plotSum = true;

  QVBoxLayout* layout=new QVBoxLayout(this);

  // set up the selection display
  m_selectionInfoDisplay = new QTextEdit(this);

  // set up the plot widget
  m_plot = new OneCurvePlot(this);
  m_plot->setYAxisLabelRotation(-90);
  m_plot->setXScale(0,1);
  m_plot->setYScale(-1.2,1.2);
  connect(m_plot,SIGNAL(showContextMenu()),this,SLOT(plotContextMenu()));
  connect(m_plot,SIGNAL(clickedAt(double,double)),this,SLOT(addPeak(double,double)));

  // Plot context menu actions
  m_sumDetectors = new QAction("Sum",this);
  m_sumDetectors->setCheckable(true);
  m_sumDetectors->setChecked(true);
  m_integrateTimeBins = new QAction("Integrate",this);
  m_integrateTimeBins->setCheckable(true);
  m_summationType = new QActionGroup(this);
  m_summationType->addAction(m_sumDetectors);
  m_summationType->addAction(m_integrateTimeBins);
  m_logY = new QAction("Y log scale",this);
  m_linearY = new QAction("Y linear scale",this);
  m_yScale = new QActionGroup(this);
  m_yScale->addAction(m_linearY);
  m_yScale->addAction(m_logY);
  m_logY->setCheckable(true);
  m_linearY->setCheckable(true);
  m_linearY->setChecked(true);
  connect(m_sumDetectors,SIGNAL(triggered()),this,SLOT(sumDetectors()));
  connect(m_integrateTimeBins,SIGNAL(triggered()),this,SLOT(integrateTimeBins()));
  connect(m_logY,SIGNAL(triggered()),m_plot,SLOT(setYLogScale()));
  connect(m_linearY,SIGNAL(triggered()),m_plot,SLOT(setYLinearScale()));

  m_unitsMapper = new QSignalMapper(this);

  m_detidUnits = new QAction("Detector ID",this);
  m_detidUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_detidUnits,DETECTOR_ID);
  connect(m_detidUnits,SIGNAL(triggered()),m_unitsMapper,SLOT(map()));

  m_lengthUnits = new QAction("Tube length",this);
  m_lengthUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_lengthUnits,LENGTH);
  connect(m_lengthUnits,SIGNAL(triggered()),m_unitsMapper,SLOT(map()));

  m_phiUnits = new QAction("Phi",this);
  m_phiUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_phiUnits,PHI);
  connect(m_phiUnits,SIGNAL(triggered()),m_unitsMapper,SLOT(map()));

  m_unitsGroup = new QActionGroup(this);
  m_unitsGroup->addAction(m_detidUnits);
  m_unitsGroup->addAction(m_lengthUnits);
  m_unitsGroup->addAction(m_phiUnits); // re #4169 disabled until fixed or removed
  connect(m_unitsMapper,SIGNAL(mapped(int)),this,SLOT(setTubeXUnits(int)));

  // Instrument display context menu actions
  m_storeCurve = new QAction("Store curve",this);
  connect(m_storeCurve,SIGNAL(triggered()),this,SLOT(storeCurve()));
  m_savePlotToWorkspace = new QAction("Save plot to workspace",this);
  connect(m_savePlotToWorkspace,SIGNAL(triggered()),this,SLOT(savePlotToWorkspace()));

  CollapsibleStack* panelStack = new CollapsibleStack(this);
  m_infoPanel = panelStack->addPanel("Selection",m_selectionInfoDisplay);
  m_plotPanel = panelStack->addPanel("Name",m_plot);

  m_activeTool = new QLabel(this);
  // set up the tool bar

  m_zoom = new QPushButton();
  m_zoom->setCheckable(true);
  m_zoom->setAutoExclusive(true);
  m_zoom->setIcon(QIcon(":/PickTools/zoom.png"));
  m_zoom->setToolTip("Zoom in and out");

  m_one = new QPushButton();
  m_one->setCheckable(true);
  m_one->setAutoExclusive(true);
  m_one->setChecked(true);
  m_one->setToolTip("Select single pixel");
  m_one->setIcon(QIcon(":/PickTools/selection-pointer.png"));

  m_tube = new QPushButton();
  m_tube->setCheckable(true);
  m_tube->setAutoExclusive(true);
  m_tube->setIcon(QIcon(":/PickTools/selection-tube.png"));
  m_tube->setToolTip("Select whole tube");

  m_rectangle = new QPushButton();
  m_rectangle->setCheckable(true);
  m_rectangle->setAutoExclusive(true);
  m_rectangle->setIcon(QIcon(":/PickTools/selection-box.png"));
  m_rectangle->setToolTip("Draw a rectangle");

  m_ellipse = new QPushButton();
  m_ellipse->setCheckable(true);
  m_ellipse->setAutoExclusive(true);
  m_ellipse->setIcon(QIcon(":/PickTools/selection-circle.png"));
  m_ellipse->setToolTip("Draw a ellipse");

  m_ring_ellipse = new QPushButton();
  m_ring_ellipse->setCheckable(true);
  m_ring_ellipse->setAutoExclusive(true);
  m_ring_ellipse->setIcon(QIcon(":/PickTools/selection-circle-ring.png"));
  m_ring_ellipse->setToolTip("Draw an elliptical ring");

  m_ring_rectangle = new QPushButton();
  m_ring_rectangle->setCheckable(true);
  m_ring_rectangle->setAutoExclusive(true);
  m_ring_rectangle->setIcon(QIcon(":/PickTools/selection-box-ring.png"));
  m_ring_rectangle->setToolTip("Draw a rectangular ring");

  m_edit = new QPushButton();
  m_edit->setCheckable(true);
  m_edit->setAutoExclusive(true);
  m_edit->setIcon(QIcon(":/PickTools/selection-edit.png"));
  m_edit->setToolTip("Edit a shape");

  m_peak = new QPushButton();
  m_peak->setCheckable(true);
  m_peak->setAutoExclusive(true);
  m_peak->setIcon(QIcon(":/PickTools/selection-peak.png"));
  m_peak->setToolTip("Add single crystal peak");

  m_peakSelect = new QPushButton();
  m_peakSelect->setCheckable(true);
  m_peakSelect->setAutoExclusive(true);
  m_peakSelect->setIcon(QIcon(":/PickTools/eraser.png"));
  m_peakSelect->setToolTip("Erase single crystal peak(s)");

  QGridLayout* toolBox = new QGridLayout();
  toolBox->addWidget(m_zoom,0,0);
  toolBox->addWidget(m_edit,0,1);
  toolBox->addWidget(m_ellipse,0,2);
  toolBox->addWidget(m_rectangle,0,3);
  toolBox->addWidget(m_ring_ellipse,0,4);
  toolBox->addWidget(m_ring_rectangle,0,5);
  toolBox->addWidget(m_one,1,0);
  toolBox->addWidget(m_tube,1,1);
  toolBox->addWidget(m_peak,1,2);
  toolBox->addWidget(m_peakSelect,1,3);
  toolBox->setColStretch(6,1);
  toolBox->setSpacing(2);
  connect(m_zoom,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_one,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_tube,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_peak,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_peakSelect,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_rectangle,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_ellipse,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_ring_ellipse,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_ring_rectangle,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_edit,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  setSelectionType();

  // lay out the widgets
  layout->addWidget(m_activeTool);
  layout->addLayout(toolBox);
  layout->addWidget(panelStack);

  setPlotCaption();
}

/**
  * Returns true if the plot can be updated when the mouse moves over detectors
  */
bool InstrumentWindowPickTab::canUpdateTouchedDetector()const
{
  return ! m_peak->isChecked();
}

/**
 * Update the miniplot for a selected detector.
 * @param detid :: ID of detector to use to update the plot.
 */
void InstrumentWindowPickTab::updatePlot(int detid)
{
  if (m_instrWindow->blocked())
  {
    m_plot->clearCurve();
    return;
  }
  if (m_plotPanel->isCollapsed()) return;

  if (detid >= 0)
  {
    if (m_one->isChecked() || m_peak->isChecked())
    {// plot spectrum of a single detector
      plotSingle(detid);
    }
    else if (m_tube->isChecked())
    {// plot integrals
      plotTube(detid);
    }
  }
  else
  {
    m_plot->clearCurve();
  }
  m_plot->recalcAxisDivs();
  m_plot->replot();
}

/**
 * Update the info window with information for a selected detector.
 * @param detid :: ID of the selected detector.
 */
void InstrumentWindowPickTab::updateSelectionInfo(int detid)
{
  if (m_freezePlot)
  {// freeze the plot for one update
    m_freezePlot = false;
    detid = m_currentDetID;
  }
  if (m_instrWindow->blocked()) 
  {
    m_selectionInfoDisplay->clear();
    return;
  }

  QString text;
  if (detid >= 0)
  {
    // collect info about selected detector and add it to text
    InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
    Mantid::Geometry::IDetector_const_sptr det;
    try
    {
        det = instrActor->getInstrument()->getDetector(detid);
    }
    catch(...)
    {
        // if this slot is called during instrument window deletion
        // expect exceptions thrown
        return;
    }

    text = "Selected detector: " + QString::fromStdString(det->getName()) + "\n";
    text += "Detector ID: " + QString::number(detid) + '\n';
    QString wsIndex;
    try {
      wsIndex = QString::number(instrActor->getWorkspaceIndex(detid));
      updatePlot(detid); // Update the plot if the detector links to some data
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      // Detector doesn't have a workspace index relating to it
      wsIndex = "None";
      m_plot->clearCurve(); // Clear the plot window
      m_plot->replot();
    }
    text += "Workspace index: " + wsIndex + '\n';
    Mantid::Kernel::V3D pos = det->getPos();
    text += "xyz: " + QString::number(pos.X()) + "," + QString::number(pos.Y()) + "," + QString::number(pos.Z())  + '\n';
    double r,t,p;
    pos.getSpherical(r,t,p);
    text += "rtp: " + QString::number(r) + "," + QString::number(t) + "," + QString::number(p)  + '\n';
    Mantid::Geometry::ICompAssembly_const_sptr parent = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(det->getParent());
    if (parent)
    {
      QString textpath;
      while (parent)
      {
        textpath="/"+QString::fromStdString(parent->getName())+textpath;
        parent=boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent->getParent());
      }
      text += "Component path:" +textpath+"/"+ QString::fromStdString(det->getName()) +'\n';
    }
    const double integrated = instrActor->getIntegratedCounts(detid);
    const QString counts = integrated == -1.0 ? "N/A" : QString::number(integrated);
    text += "Counts: " + counts + '\n';
    QString xUnits;
    if (m_selectionType > SingleDetectorSelection && !m_plotSum)
    {
      switch(m_tubeXUnits)
      {
      case DETECTOR_ID: xUnits = "Detector ID"; break;
      case LENGTH: xUnits = "Length"; break;
      case PHI: xUnits = "Phi"; break;
      default: xUnits = "Detector ID";
      }
    }
    else
    {
      xUnits = QString::fromStdString(instrActor->getWorkspace()->getAxis(0)->unit()->caption());
    }
    text += "X units: " + xUnits + '\n';
  }
  else
  {
    m_plot->clearCurve(); // Clear the plot window
    m_plot->replot();
  }

  // display info about peak overlays
  text += getNonDetectorInfo();

  if ( !text.isEmpty() )
  {
      m_selectionInfoDisplay->setText(text);
  }
  else
  {
      m_selectionInfoDisplay->clear();
  }
}

/**
 * Display the miniplot's context menu.
 */
void InstrumentWindowPickTab::plotContextMenu()
{
  QMenu context(this);
  
  if (m_selectionType > SingleDetectorSelection)
  {// only for multiple detector selectors
    context.addActions(m_summationType->actions());
    if ( m_selectionType == Draw )
    {
        m_sumDetectors->setChecked(true);
        m_integrateTimeBins->setEnabled(false);
    }
    else
    {
        m_sumDetectors->setChecked(m_plotSum);
        m_integrateTimeBins->setEnabled(true);
    }
    context.addSeparator();
  }

  if (m_plot->hasStored())
  {
    // the remove menu
    QMenu *removeCurves = new QMenu("Remove",this);
    QSignalMapper *signalMapper = new QSignalMapper(this);
    QStringList labels = m_plot->getLabels();
    foreach(QString label,labels)
    {
      QColor c = m_plot->getCurveColor(label);
      QPixmap pixmap(16,2);
      pixmap.fill(c);
      QAction *remove = new QAction(QIcon(pixmap),label,removeCurves);
      removeCurves->addAction(remove);
      connect(remove,SIGNAL(triggered()),signalMapper,SLOT(map()));
      signalMapper->setMapping(remove,label);
    }
    connect(signalMapper, SIGNAL(mapped(const QString &)),
             this, SLOT(removeCurve(const QString &)));
    context.addMenu(removeCurves);
  }

  // the axes menu
  QMenu* axes = new QMenu("Axes",this);
  axes->addActions(m_yScale->actions());
  if (m_plot->isYLogScale())
  {
    m_logY->setChecked(true);
  }
  else
  {
    m_linearY->setChecked(true);
  }

  // Tube x units menu options
  if (m_selectionType > SingleDetectorSelection && !m_plotSum)
  {
    axes->addSeparator();
    axes->addActions(m_unitsGroup->actions());
    switch(m_tubeXUnits)
    {
    case DETECTOR_ID: m_detidUnits->setChecked(true); break;
    case LENGTH: m_lengthUnits->setChecked(true); break;
    case PHI: m_phiUnits->setChecked(true); break;
    default: m_detidUnits->setChecked(true);
    }
  }
  context.addMenu(axes);

  // save plot to workspace
  if (m_plot->hasStored() || m_plot->hasCurve())
  {
    context.addAction(m_savePlotToWorkspace);
  }

  // show menu
  context.exec(QCursor::pos());
}

/**
 * Update the plot caption. The captions shows the selection type.
 */
void InstrumentWindowPickTab::setPlotCaption()
{
  QString caption;
  if (m_selectionType < SingleDetectorSelection)
  {
    caption = "Plotting detector spectra";
  }
  else if (m_plotSum)
  {
    caption = "Plotting sum";
  }
  else
  {
    caption = "Plotting integral";
  }
  m_plotPanel->setCaption(caption);
}

/**
 * Switch to the detectors summing regime.
 */
void InstrumentWindowPickTab::sumDetectors()
{
  m_plotSum = true;
  m_plot->clearAll();
  m_plot->replot();
  setPlotCaption();
}

/**
 * Switch to the time bin integration regime.
 */
void InstrumentWindowPickTab::integrateTimeBins()
{
  m_plotSum = false;
  m_plot->clearAll();
  m_plot->replot();
  setPlotCaption();
}

/**
 * Update the tab to display info for a new detector.
 * @param detid :: ID of the new detector.
 */
void InstrumentWindowPickTab::updatePick(int detid)
{
  updateSelectionInfo(detid); // Also calls updatePlot
  m_currentDetID = detid;
}

/**
 * Plot data for a detector.
 * @param detid :: ID of the detector to be plotted.
 */
void InstrumentWindowPickTab::plotSingle(int detid)
{

  std::vector<double> x,y;
  prepareDataForSinglePlot(detid,x,y);

  m_plot->clearPeakLabels();
  // set the data 
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()), m_instrWindow->getInstrumentActor()->getWorkspace()->getAxis(0)->unit()->unitID());
  m_plot->setLabel("Detector " + QString::number(detid));

  // find any markers
  auto surface = getSurface();
  if (surface)
  {
    QList<PeakMarker2D*> markers = surface->getMarkersWithID(detid);
    foreach(PeakMarker2D* marker,markers)
    {
      m_plot->addPeakLabel(marker);
    }
  }
}

/**
 * Plot data integrated either over the detectors in a tube or over time bins.
 * If m_plotSum == true the miniplot displays the accumulated data in a tube against time of flight.
 * If m_plotSum == false the miniplot displays the data integrated over the time bins. The values are
 * plotted against the length of the tube, but the units on the x-axis can be one of the following:
 *   DETECTOR_ID
 *   LENGTH
 *   PHI
 * The units can be set with setTubeXUnits(...) method.
 * @param detid :: A detector id. The miniplot will display data for a component containing the detector 
 *   with this id.
 */
void InstrumentWindowPickTab::plotTube(int detid)
{
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
  Mantid::Geometry::IDetector_const_sptr det = instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  if (parent && ass)
  {
    if (m_plotSum) // plot sums over detectors vs time bins
    {
      plotTubeSums(detid);
    }
    else // plot detector integrals vs detID or a function of detector position in the tube
    {
      plotTubeIntegrals(detid);
    }
  }
  else
  {
    m_plot->clearCurve();
  }
}

/**
 * Plot the accumulated data in a tube against time of flight.
 * @param detid :: A detector id. The miniplot will display data for a component containing the detector 
 *   with this id.
 */
void InstrumentWindowPickTab::plotTubeSums(int detid)
{
  std::vector<double> x,y;
  prepareDataForSumsPlot(detid,x,y);
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::Geometry::IDetector_const_sptr det = instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  QString label = QString::fromStdString(parent->getName()) + " (" + QString::number(detid) + ") Sum"; 
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()), m_instrWindow->getInstrumentActor()->getWorkspace()->getAxis(0)->unit()->unitID());
  m_plot->setLabel(label);
}

/**
 * Plot the data integrated over the time bins. The values are
 * plotted against the length of the tube, but the units on the x-axis can be one of the following:
 *   DETECTOR_ID
 *   LENGTH
 *   PHI
 * The units can be set with setTubeXUnits(...) method.
 * @param detid :: A detector id. The miniplot will display data for a component containing the detector 
 *   with this id.
 */
void InstrumentWindowPickTab::plotTubeIntegrals(int detid)
{
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::Geometry::IDetector_const_sptr det = instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  // curve label: "tube_name (detid) Integrals"
  // detid is included to distiguish tubes with the same name
  QString label = QString::fromStdString(parent->getName()) + " (" + QString::number(detid) + ") Integrals"; 
  label += "/" + getTubeXUnitsName(m_tubeXUnits);
  std::vector<double> x,y;
  prepareDataForIntegralsPlot(detid,x,y);
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()));
  m_plot->setLabel(label);
}

/**
 * Set the selection type according to which tool button is checked.
 */
void InstrumentWindowPickTab::setSelectionType()
{
  ProjectionSurface::InteractionMode surfaceMode = ProjectionSurface::PickSingleMode;
  if (m_zoom->isChecked())
  {
    m_selectionType = Single;
    m_activeTool->setText("Tool: Navigation");
    surfaceMode = ProjectionSurface::MoveMode;
  }
  else if (m_one->isChecked())
  {
    m_selectionType = Single;
    m_activeTool->setText("Tool: Pixel selection");
    surfaceMode = ProjectionSurface::PickSingleMode;
  }
  else if (m_tube->isChecked())
  {
    m_selectionType = Tube;
    m_activeTool->setText("Tool: Tube/bank selection");
    surfaceMode = ProjectionSurface::PickTubeMode;
  }
  else if (m_peak->isChecked())
  {
    m_selectionType = AddPeak;
    m_activeTool->setText("Tool: Add a single crystal peak");
    surfaceMode = ProjectionSurface::AddPeakMode;
  }
  else if (m_peakSelect->isChecked())
  {
    m_selectionType = ErasePeak;
    m_activeTool->setText("Tool: Erase crystal peak(s)");
    surfaceMode = ProjectionSurface::EraseMode;
  }
  else if (m_rectangle->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Rectangle");
    surfaceMode = ProjectionSurface::DrawMode;
    m_instrWindow->getSurface()->startCreatingShape2D("rectangle",Qt::green,QColor(255,255,255,80));
  }
  else if (m_ellipse->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Ellipse");
    surfaceMode = ProjectionSurface::DrawMode;
    m_instrWindow->getSurface()->startCreatingShape2D("ellipse",Qt::green,QColor(255,255,255,80));
  }
  else if (m_ring_ellipse->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Elliptical ring");
    surfaceMode = ProjectionSurface::DrawMode;
    m_instrWindow->getSurface()->startCreatingShape2D("ring ellipse",Qt::green,QColor(255,255,255,80));
  }
  else if (m_ring_rectangle->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Rectangular ring");
    surfaceMode = ProjectionSurface::DrawMode;
    m_instrWindow->getSurface()->startCreatingShape2D("ring rectangle",Qt::green,QColor(255,255,255,80));
  }
  else if (m_edit->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Shape editing");
    surfaceMode = ProjectionSurface::DrawMode;
  }
  auto surface = m_instrWindow->getSurface();
  if ( surface ) 
  {
    surface->setInteractionMode( surfaceMode );
    auto interactionMode = surface->getInteractionMode();
    if ( interactionMode == ProjectionSurface::DrawMode || interactionMode == ProjectionSurface::MoveMode )
    {
        updatePlotMultipleDetectors();
    }
    else
    {
        m_plot->clearAll();
        m_plot->replot();
        setPlotCaption();
    }
  }
  m_instrWindow->updateInfoText();
}

/**
  * Add a peak to the single crystal peak table.
  * @param x :: Time of flight
  * @param y :: Peak height (counts)
  */
void InstrumentWindowPickTab::addPeak(double x,double y)
{
  if (!m_peak->isChecked() ||  m_currentDetID < 0) return;

  try
  {
      Mantid::API::IPeaksWorkspace_sptr tw = m_instrWindow->getSurface()->getEditPeaksWorkspace();
      InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
      Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
      std::string peakTableName;
      bool newPeaksWorkspace = false;
      if ( tw )
      {
          peakTableName = tw->name();
      }
      else
      {
          peakTableName = "SingleCrystalPeakTable";
          // This does need to get the instrument from the workspace as it's doing calculations
          // .....and this method should be an algorithm! Or at least somewhere different to here.
          Mantid::Geometry::Instrument_const_sptr instr = ws->getInstrument();

          if (! Mantid::API::AnalysisDataService::Instance().doesExist(peakTableName))
          {
              tw = Mantid::API::WorkspaceFactory::Instance().createPeaks("PeaksWorkspace");
              tw->setInstrument(instr);
              Mantid::API::AnalysisDataService::Instance().add(peakTableName,tw);
              newPeaksWorkspace = true;
          }
          else
          {
              tw = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(peakTableName));
              if (!tw)
              {
                  QMessageBox::critical(this,"Mantid - Error","Workspace " + QString::fromStdString(peakTableName) + " is not a TableWorkspace");
                  return;
              }
          }
          auto surface = boost::dynamic_pointer_cast<UnwrappedSurface>( m_instrWindow->getSurface() );
          if ( surface )
          {
              surface->setPeaksWorkspace(boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(tw));
          }
      }

      // Run the AddPeak algorithm
      auto alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("AddPeak");
      alg->setPropertyValue( "RunWorkspace", ws->name() );
      alg->setPropertyValue( "PeaksWorkspace", peakTableName );
      alg->setProperty( "DetectorID", m_currentDetID );
      alg->setProperty( "TOF", x );
      alg->setProperty( "Height", instrActor->getIntegratedCounts(m_currentDetID) );
      alg->setProperty( "BinCount", y );
      alg->execute();

      // if data WS has UB copy it to the new peaks workspace
      if ( newPeaksWorkspace && ws->sample().hasOrientedLattice() )
      {
          auto UB = ws->sample().getOrientedLattice().getUB();
          auto lattice = new Mantid::Geometry::OrientedLattice;
          lattice->setUB(UB);
          tw->mutableSample().setOrientedLattice(lattice);
      }

      // if there is a UB available calculate HKL for the new peak
      if ( tw->sample().hasOrientedLattice() )
      {
          auto alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("CalculatePeaksHKL");
          alg->setPropertyValue( "PeaksWorkspace", peakTableName );
          alg->execute();
      }
  }
  catch(std::exception& e)
  {
      QMessageBox::critical(this,"MantidPlot -Error",
                            "Cannot create a Peak object because of the error:\n"+QString(e.what()));
  }

}

/**
 * Respond to the show event.
 */
void InstrumentWindowPickTab::showEvent (QShowEvent *)
{
  // Make the state of the display view consistent with the current selection type
  setSelectionType();
  // make sure picking updated
  m_instrWindow->updateInstrumentView(true);
  m_instrWindow->getSurface()->changeBorderColor( getShapeBorderColor() );
}

/**
 * Keep current curve permanently displayed on the plot.
 */
void InstrumentWindowPickTab::storeCurve()
{
  m_plot->store();
}

/**
 * Remove a stored curve.
 * @param label :: The label of the curve to remove
 */
void InstrumentWindowPickTab::removeCurve(const QString & label)
{
  m_plot->removeCurve(label);
  m_plot->replot();
}

/**
 * Set the x units for the integrated tube plot.
 * @param units :: The x units in terms of TubeXUnits.
 */
void InstrumentWindowPickTab::setTubeXUnits(int units)
{
  if (units < 0 || units >= NUMBER_OF_UNITS) return;
  m_tubeXUnits = static_cast<TubeXUnits>(units);
  m_plot->clearAll();
  m_plot->replot();
}

/**
 * Prepare data for plotting a spectrum of a single detector.
 * @param detid :: ID of the detector to be plotted.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void InstrumentWindowPickTab::prepareDataForSinglePlot(
  int detid,
  std::vector<double>&x,
  std::vector<double>&y,
  std::vector<double>* err)
{
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
  size_t wi;
  try {
    wi = instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    return; // Detector doesn't have a workspace index relating to it
  }
  // get the data
  const Mantid::MantidVec& X = ws->readX(wi);
  const Mantid::MantidVec& Y = ws->readY(wi);
  const Mantid::MantidVec& E = ws->readE(wi);

  // find min and max for x
  size_t imin,imax;
  instrActor->getBinMinMaxIndex(wi,imin,imax);

  x.assign(X.begin() + imin,X.begin() + imax);
  y.assign(Y.begin() + imin,Y.begin() + imax);
  if ( ws->isHistogramData() )
  {
    // calculate the bin centres
    std::transform(x.begin(),x.end(),X.begin() + imin + 1,x.begin(),std::plus<double>());
    std::transform(x.begin(),x.end(),x.begin(),std::bind2nd(std::divides<double>(),2.0));
  }

  if (err)
  {
    err->assign(E.begin() + imin,E.begin() + imax);
  }
}

/**
 * Prepare data for plotting accumulated data in a tube against time of flight.
 * @param detid :: A detector id. The miniplot will display data for a component containing the detector 
 *   with this id.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void InstrumentWindowPickTab::prepareDataForSumsPlot(
  int detid,
  std::vector<double>&x,
  std::vector<double>&y,
  std::vector<double>* err)
{
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
  Mantid::Geometry::IDetector_const_sptr det = instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  size_t wi;
  try {
    wi = instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    return; // Detector doesn't have a workspace index relating to it
  }
  size_t imin,imax;
  instrActor->getBinMinMaxIndex(wi,imin,imax);

  const Mantid::MantidVec& X = ws->readX(wi);
  x.assign(X.begin() + imin, X.begin() + imax);
  if ( ws->isHistogramData() )
  {
    // calculate the bin centres
    std::transform(x.begin(),x.end(),X.begin() + imin + 1,x.begin(),std::plus<double>());
    std::transform(x.begin(),x.end(),x.begin(),std::bind2nd(std::divides<double>(),2.0));
  }
  y.resize(x.size(),0);
  if (err)
  {
    err->resize(x.size(),0);
  }

  const int n = ass->nelements();
  for(int i = 0; i < n; ++i)
  {
    Mantid::Geometry::IDetector_sptr idet = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*ass)[i]);
    if (idet)
    {
      try {
        size_t index = instrActor->getWorkspaceIndex(idet->getID());
        const Mantid::MantidVec& Y = ws->readY(index);
        std::transform(y.begin(),y.end(),Y.begin() + imin,y.begin(),std::plus<double>());
        if (err)
        {
          const Mantid::MantidVec& E = ws->readE(index);
          std::vector<double> tmp;
          tmp.assign(E.begin() + imin,E.begin() + imax);
          std::transform(tmp.begin(),tmp.end(),tmp.begin(),tmp.begin(),std::multiplies<double>());
          std::transform(err->begin(),err->end(),tmp.begin(),err->begin(),std::plus<double>());
        }
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        continue; // Detector doesn't have a workspace index relating to it
      }
    }
  }

  if (err)
  {
    std::transform(err->begin(),err->end(),err->begin(),Sqrt());
  }
}

/**
 * Prepare data for plotting the data integrated over the time bins. The values are
 * plotted against the length of the tube, but the units on the x-axis can be one of the following:
 *   DETECTOR_ID
 *   LENGTH
 *   PHI
 * The units can be set with setTubeXUnits(...) method.
 * @param detid :: A detector id. The miniplot will display data for a component containing the detector 
 *   with this id.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void InstrumentWindowPickTab::prepareDataForIntegralsPlot(
  int detid,
  std::vector<double>&x,
  std::vector<double>&y,
  std::vector<double>* err)
{
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();

  // Does the instrument definition specify that psi should be offset.
  std::vector<std::string> parameters = ws->getInstrument()->getStringParameter("offset-phi");
  const bool bOffsetPsi = (!parameters.empty()) && std::find(parameters.begin(), parameters.end(), "Always") != parameters.end();

  Mantid::Geometry::IDetector_const_sptr det = instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  size_t wi;
  try {
    wi = instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    return; // Detector doesn't have a workspace index relating to it
  }
  // imin and imax give the bin integration range
  size_t imin,imax;
  instrActor->getBinMinMaxIndex(wi,imin,imax);

  const int n = ass->nelements();
  if (n == 0)
  {
    // don't think it's ever possible but...
    throw std::runtime_error("PickTab miniplot: empty instrument assembly");
  }
  // collect and sort xy pairs in xymap
  std::map<double,double> xymap,errmap;
  // get the first detector in the tube for lenth calculation
  Mantid::Geometry::IDetector_sptr idet0 = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*ass)[0]);
  for(int i = 0; i < n; ++i)
  {
    Mantid::Geometry::IDetector_sptr idet = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*ass)[i]);
    if (idet)
    {
      try {
        const int id = idet->getID();
        double xvalue = 0;
        switch(m_tubeXUnits)
        {
        case LENGTH: xvalue = idet->getDistance(*idet0); break;
        case PHI: xvalue = bOffsetPsi ? idet->getPhiOffset(M_PI) : idet->getPhi(); break;
        default: xvalue = static_cast<double>(id);
        }
        size_t index = instrActor->getWorkspaceIndex(id);
        const Mantid::MantidVec& Y = ws->readY(index);
        double sum = std::accumulate(Y.begin() + imin,Y.begin() + imax,0);
        xymap[xvalue] = sum;
        if (err)
        {
          const Mantid::MantidVec& E = ws->readE(index);
          std::vector<double> tmp(imax - imin);
          // take squares of the errors
          std::transform(E.begin() + imin,E.begin() + imax,E.begin() + imin,tmp.begin(),std::multiplies<double>());
          // sum them
          double sum = std::accumulate(tmp.begin(),tmp.end(),0);
          // take sqrt
          errmap[xvalue] = sqrt(sum);
        }
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        continue; // Detector doesn't have a workspace index relating to it
      }
    }
  }
  if (!xymap.empty())
  {
    // set the plot curve data
    x.resize(xymap.size());
    y.resize(xymap.size());
    std::map<double,double>::const_iterator xy = xymap.begin();
    for(size_t i = 0; xy != xymap.end(); ++xy,++i)
    {
      x[i] = xy->first;
      y[i] = xy->second;
    }
    if (err)
    {
      err->resize(errmap.size());
      std::map<double,double>::const_iterator e = errmap.begin();
      for(size_t i = 0; e != errmap.end(); ++e,++i)
      {
        (*err)[i] = e->second;
      }
    }
  }
  else
  {
    x.clear();
    y.clear();
    if (err)
    {
      err->clear();
    }
  }
}

/**
 * Return value of TubeXUnits for its symbolic name.
 * @param name :: Symbolic name of the units, caseless: Detector_ID, Length, Phi
 */
InstrumentWindowPickTab::TubeXUnits InstrumentWindowPickTab::getTubeXUnits(const QString& name) const
{
  QString caseless_name = name.toUpper();
  if (caseless_name == "LENGTH")
  {
    return LENGTH;
  }
  if (caseless_name == "PHI")
  {
    return PHI;
  }
  return DETECTOR_ID;
}

/**
 * Return symbolic name of a TubeXUnit.
 * @param unit :: One of TubeXUnits.
 * @return :: Symbolic name of the units, caseless: Detector_ID, Length, Phi
 */
QString InstrumentWindowPickTab::getTubeXUnitsName(InstrumentWindowPickTab::TubeXUnits unit) const
{
  switch(unit)
  {
  case LENGTH: return "Length";
  case PHI: return "Phi";
  default: return "Detector_ID";
  }
  return "Detector_ID";
}

/**
  * Return non-detector info to be displayed in the selection info display.
  */
QString InstrumentWindowPickTab::getNonDetectorInfo()
{
    QString text;
    QStringList overlays = m_instrWindow->getSurface()->getPeaksWorkspaceNames();
    if ( !overlays.isEmpty() )
    {
        text += "Peaks:\n" + overlays.join("\n") + "\n";
    }
    return text;
}

/**
 * Get the color of the overlay shapes in this tab.
 * @return
 */
QColor InstrumentWindowPickTab::getShapeBorderColor() const
{
    return QColor( Qt::green );
}


/**
 * Save data plotted on the miniplot into a MatrixWorkspace.
 */
void InstrumentWindowPickTab::savePlotToWorkspace()
{
  if (!m_plot->hasCurve() && !m_plot->hasStored())
  {
    // nothing to save
    return;
  }
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr parentWorkspace = instrActor->getWorkspace();
  // interpret curve labels and reconstruct the data to be saved
  QStringList labels = m_plot->getLabels();
  if (m_plot->hasCurve())
  {
    labels << m_plot->label();
  }
  std::vector<double> X,Y,E;
  size_t nbins = 0;
  // to keep det ids for spectrum-detector mapping in the output workspace
  std::vector<Mantid::detid_t> detids;
  // unit id for x vector in the created workspace
  std::string unitX;
  foreach(QString label,labels)
  {
    std::vector<double> x,y,e;
    // split the label to get the detector id and selection type 
    QStringList parts = label.split(QRegExp("[()]"));
    if ( label == "multiple" )
    {
        QList<int> dets;
        getSurface()->getMaskedDetectors( dets );
        m_instrWindow->getInstrumentActor()->sumDetectors( dets, x, y );
        unitX = parentWorkspace->getAxis(0)->unit()->unitID();
    }
    else if (parts.size() == 3)
    {
      int detid = parts[1].toInt();
      QString SumOrIntegral = parts[2].trimmed();
      if (SumOrIntegral == "Sum")
      {
        prepareDataForSumsPlot(detid,x,y,&e);
        unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      }
      else
      {
        prepareDataForIntegralsPlot(detid,x,y,&e);
        unitX = SumOrIntegral.split('/')[1].toStdString();
      }
    }
    else if (parts.size() == 1)
    {
      // second word is detector id
      int detid = parts[0].split(QRegExp("\\s+"))[1].toInt();
      prepareDataForSinglePlot(detid,x,y,&e);
      unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      // save det ids for the output workspace
      detids.push_back(static_cast<Mantid::detid_t>(detid));
    }
    else
    {
      continue;
    }
    if (!x.empty() && x.size() == y.size())
    {
      if (nbins > 0 && x.size() != nbins)
      {
        QMessageBox::critical(this,"MantidPlot - Error","Curves have different sizes.");
        return;
      }
      else
      {
        nbins = x.size();
      }
      X.insert(X.end(),x.begin(),x.end());
      Y.insert(Y.end(),y.begin(),y.end());
      E.insert(E.end(),e.begin(),e.end());
    }
  }
  // call CreateWorkspace algorithm. Created worksapce will have name "Curves"
  if (!X.empty())
  {
    E.resize(X.size(),1.0);
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmFactory::Instance().create("CreateWorkspace",-1);
    alg->initialize();
    alg->setPropertyValue("OutputWorkspace","Curves");
    alg->setProperty("DataX",X);
    alg->setProperty("DataY",Y);
    alg->setProperty("DataE",E);
    alg->setProperty("NSpec",static_cast<int>(X.size()/nbins));
    alg->setProperty("UnitX",unitX);
    alg->setPropertyValue("ParentWorkspace",parentWorkspace->name());
    alg->execute();

    if (!detids.empty())
    {
      // set up spectra - detector mapping
      Mantid::API::MatrixWorkspace_sptr ws = 
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("Curves"));
      if (!ws)
      {
        throw std::runtime_error("Failed to create Curves workspace");
      }

      if (detids.size() == ws->getNumberHistograms())
      {
        size_t i = 0;
        for(std::vector<Mantid::detid_t>::const_iterator id = detids.begin(); id != detids.end(); ++id,++i)
        {
          Mantid::API::ISpectrum * spec = ws->getSpectrum(i);
          if (!spec)
          {
            throw std::runtime_error("Spectrum not found");
          }
          spec->setDetectorID(*id);
        }
      }
      
    } // !detids.empty()
  }
}

/**
 * Do something when the time bin integraion range has changed.
 */
void InstrumentWindowPickTab::changedIntegrationRange(double,double)
{
  m_plot->clearAll();
  m_plot->replot();
  auto surface = m_instrWindow->getSurface();
  if ( surface )
  {
    auto interactionMode = surface->getInteractionMode();
    if ( interactionMode == ProjectionSurface::DrawMode || interactionMode == ProjectionSurface::MoveMode )
    {
        updatePlotMultipleDetectors();
    }
  }
}

/**
 * Clears the miniplot if mouse leaves the instrument display and Peak selection isn't on.
 */
void InstrumentWindowPickTab::mouseLeftInstrmentDisplay()
{
  if (m_selectionType < ErasePeak)
  {
    updatePick(-1);
  }
}

void InstrumentWindowPickTab::initSurface()
{
    ProjectionSurface *surface = getSurface().get();
    connect(surface,SIGNAL(singleDetectorTouched(int)),this,SLOT(singleDetectorTouched(int)));
    connect(surface,SIGNAL(singleDetectorPicked(int)),this,SLOT(singleDetectorPicked(int)));
    connect(surface,SIGNAL(peaksWorkspaceAdded()),this,SLOT(updateSelectionInfoDisplay()));
    connect(surface,SIGNAL(peaksWorkspaceDeleted()),this,SLOT(updateSelectionInfoDisplay()));
    connect(surface,SIGNAL(shapeCreated()),this,SLOT(shapeCreated()));
    connect(surface,SIGNAL(shapeChangeFinished()),this,SLOT(updatePlotMultipleDetectors()));
    connect(surface,SIGNAL(shapesCleared()),this,SLOT(updatePlotMultipleDetectors()));
    connect(surface,SIGNAL(shapesRemoved()),this,SLOT(updatePlotMultipleDetectors()));
    Projection3D *p3d = dynamic_cast<Projection3D*>( surface );
    if ( p3d )
    {
        connect(p3d,SIGNAL(finishedMove()),this,SLOT(updatePlotMultipleDetectors()));
    }
}

/**
  * Save tab's persistent settings to the provided QSettings instance
  */
void InstrumentWindowPickTab::saveSettings(QSettings &settings) const
{
    settings.setValue("TubeXUnits",getTubeXUnits());
}

/**
 * Restore (read and apply) tab's persistent settings from the provided QSettings instance
 */
void InstrumentWindowPickTab::loadSettings(const QSettings &settings)
{
    int unitsNum = settings.value("TubeXUnits",0).toInt();
    m_tubeXUnits = TubeXUnits( unitsNum );
}

/**
  * Fill in the context menu.
  * @param context :: A menu to fill.
  */
bool InstrumentWindowPickTab::addToDisplayContextMenu(QMenu &context) const
{
    m_freezePlot = true;
    bool res = false;
    if (m_plot->hasCurve())
    {
      context.addAction(m_storeCurve);
      res = true;
    }
    if (m_plot->hasStored() || m_plot->hasCurve())
    {
      context.addAction(m_savePlotToWorkspace);
      res = true;
    }
    return res;
}

/**
 * Select a tool on the tab
 * @param tool One of the enumerated tool types, @see ToolType
 */
void InstrumentWindowPickTab::selectTool(const ToolType tool)
{
  switch(tool)
  {
  case Zoom: m_zoom->setChecked(true);
    break;
  case PixelSelect: m_one->setChecked(true);
    break;
  case TubeSelect: m_tube->setChecked(true);
    break;
  case PeakSelect: m_peak->setChecked(true);
    break;
  case PeakErase: m_peakSelect->setChecked(true);
    break;
  case DrawRectangle: m_rectangle->setChecked(true);
    break;
  case DrawEllipse: m_ellipse->setChecked(true);
    break;
  case EditShape: m_edit->setChecked(true);
    break;
  default: throw std::invalid_argument("Invalid tool type.");
  }
  setSelectionType();
}


void InstrumentWindowPickTab::singleDetectorTouched(int detid)
{
  if (canUpdateTouchedDetector())
  {
    updatePick(detid);
  }
}

void InstrumentWindowPickTab::singleDetectorPicked(int detid)
{
    updatePick(detid);
}

/**
  * Update the selection display using currently selected detector.
  * Updates non-detector information on it.
  */
void InstrumentWindowPickTab::updateSelectionInfoDisplay()
{
    updateSelectionInfo(m_currentDetID);
}

/**
 * Respond to the shapeCreated signal from the surface.
 */
void InstrumentWindowPickTab::shapeCreated()
{
    selectTool( EditShape );
}

/**
 * Update the mini-plot with information from multiple detector
 * selected with drawn shapes.
 */
void InstrumentWindowPickTab::updatePlotMultipleDetectors()
{
    if ( !isVisible() ) return;
    ProjectionSurface &surface = *getSurface();
    m_plot->clearAll();
    if ( !surface.hasMasks() )
    {
        m_plot->replot();
        return;
    }
    QList<int> dets;
    surface.getMaskedDetectors( dets );
    std::vector<double> x,y;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_instrWindow->getInstrumentActor()->sumDetectors( dets, x, y );
    QApplication::restoreOverrideCursor();
    if ( !x.empty() )
    {
        m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()), m_instrWindow->getInstrumentActor()->getWorkspace()->getAxis(0)->unit()->unitID());
    }
    m_plot->setLabel("multiple");
    m_plot->replot();
}

