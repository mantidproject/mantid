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
#include "MantidKernel/DynamicFactory.h"
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
//m_tubeXUnits(DETECTOR_ID),
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
  m_unitsMapper->setMapping(m_detidUnits,DetectorPlotController::DETECTOR_ID);
  connect(m_detidUnits,SIGNAL(triggered()),m_unitsMapper,SLOT(map()));

  m_lengthUnits = new QAction("Tube length",this);
  m_lengthUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_lengthUnits,DetectorPlotController::LENGTH);
  connect(m_lengthUnits,SIGNAL(triggered()),m_unitsMapper,SLOT(map()));

  m_phiUnits = new QAction("Phi",this);
  m_phiUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_phiUnits,DetectorPlotController::PHI);
  connect(m_phiUnits,SIGNAL(triggered()),m_unitsMapper,SLOT(map()));

  m_outOfPlaneAngleUnits = new QAction("Out of plane angle",this);
  m_outOfPlaneAngleUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_outOfPlaneAngleUnits,DetectorPlotController::OUT_OF_PLANE_ANGLE);
  connect(m_outOfPlaneAngleUnits,SIGNAL(triggered()),m_unitsMapper,SLOT(map()));

  m_unitsGroup = new QActionGroup(this);
  m_unitsGroup->addAction(m_detidUnits);
  m_unitsGroup->addAction(m_lengthUnits);
  m_unitsGroup->addAction(m_phiUnits); // re #4169 disabled until fixed or removed
  m_unitsGroup->addAction(m_outOfPlaneAngleUnits);
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

  // lay out the widgets
  layout->addWidget(m_activeTool);
  layout->addLayout(toolBox);
  layout->addWidget(panelStack);

}

/**
  * Returns true if the plot can be updated when the mouse moves over detectors
  */
bool InstrumentWindowPickTab::canUpdateTouchedDetector()const
{
  return ! m_peak->isChecked();
}


/**
 * Display the miniplot's context menu.
 */
void InstrumentWindowPickTab::plotContextMenu()
{
  QMenu context(this);

  auto plotType = m_plotController->getPlotType();
  
  if ( plotType == DetectorPlotController::TubeSum || 
    plotType == DetectorPlotController::TubeIntegral )
  {
    // only for multiple detector selectors
    context.addActions(m_summationType->actions());
    m_sumDetectors->setChecked( plotType == DetectorPlotController::TubeSum );
    m_integrateTimeBins->setChecked( plotType != DetectorPlotController::TubeSum );
    m_integrateTimeBins->setEnabled(true);
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
  if ( plotType == DetectorPlotController::TubeIntegral )
  {
    axes->addSeparator();
    axes->addActions(m_unitsGroup->actions());
    auto tubeXUnits = m_plotController->getTubeXUnits();
    switch(tubeXUnits)
    {
    case DetectorPlotController::DETECTOR_ID:         m_detidUnits->setChecked(true); break;
    case DetectorPlotController::LENGTH:              m_lengthUnits->setChecked(true); break;
    case DetectorPlotController::PHI:                 m_phiUnits->setChecked(true); break;
    case DetectorPlotController::OUT_OF_PLANE_ANGLE:  m_outOfPlaneAngleUnits->setChecked(true); break;
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
  m_plotPanel->setCaption(m_plotController->getPlotCaption());
}

/**
 * Switch to the detectors summing regime.
 */
void InstrumentWindowPickTab::sumDetectors()
{
  //m_plotSum = true;
  m_plotController->setPlotType( DetectorPlotController::TubeSum  );
  m_plot->clearAll();
  m_plot->replot();
  setPlotCaption();
}

/**
 * Switch to the time bin integration regime.
 */
void InstrumentWindowPickTab::integrateTimeBins()
{
  //m_plotSum = false;
  m_plotController->setPlotType( DetectorPlotController::TubeIntegral );
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
  //updateSelectionInfo(detid); // Also calls updatePlot
  m_currentDetID = detid;
}

/**
 * Set the selection type according to which tool button is checked.
 */
void InstrumentWindowPickTab::setSelectionType()
{
  ProjectionSurface::InteractionMode surfaceMode = ProjectionSurface::PickSingleMode;
  auto plotType = m_plotController->getPlotType();
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
    plotType = DetectorPlotController::Single;
  }
  else if (m_tube->isChecked())
  {
    m_selectionType = Tube;
    m_activeTool->setText("Tool: Tube/bank selection");
    surfaceMode = ProjectionSurface::PickTubeMode;
    if ( plotType < DetectorPlotController::TubeSum )
    {
      plotType = DetectorPlotController::TubeSum;
    }
  }
  else if (m_peak->isChecked())
  {
    m_selectionType = AddPeak;
    m_activeTool->setText("Tool: Add a single crystal peak");
    surfaceMode = ProjectionSurface::AddPeakMode;
    plotType = DetectorPlotController::Single;
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
    plotType = DetectorPlotController::Single;
    m_instrWindow->getSurface()->startCreatingShape2D("rectangle",Qt::green,QColor(255,255,255,80));
  }
  else if (m_ellipse->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Ellipse");
    surfaceMode = ProjectionSurface::DrawMode;
    plotType = DetectorPlotController::Single;
    m_instrWindow->getSurface()->startCreatingShape2D("ellipse",Qt::green,QColor(255,255,255,80));
  }
  else if (m_ring_ellipse->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Elliptical ring");
    surfaceMode = ProjectionSurface::DrawMode;
    plotType = DetectorPlotController::Single;
    m_instrWindow->getSurface()->startCreatingShape2D("ring ellipse",Qt::green,QColor(255,255,255,80));
  }
  else if (m_ring_rectangle->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Rectangular ring");
    surfaceMode = ProjectionSurface::DrawMode;
    plotType = DetectorPlotController::Single;
    m_instrWindow->getSurface()->startCreatingShape2D("ring rectangle",Qt::green,QColor(255,255,255,80));
  }
  else if (m_edit->isChecked())
  {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Shape editing");
    surfaceMode = ProjectionSurface::DrawMode;
    plotType = DetectorPlotController::Single;
  }
  m_plotController->setPlotType( plotType );
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
    }
    setPlotCaption();
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
  if (units < 0 || units >= DetectorPlotController::NUMBER_OF_UNITS) return;
  auto tubeXUnits = static_cast<DetectorPlotController::TubeXUnits>(units);
  m_plotController->setTubeXUnits(tubeXUnits);
  m_plot->clearAll();
  m_plot->replot();
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
    connect(surface,SIGNAL(singleComponentTouched(size_t)),this,SLOT(singleComponentTouched(size_t)));
    connect(surface,SIGNAL(singleComponentPicked(size_t)),this,SLOT(singleComponentPicked(size_t)));
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
    m_infoController = new ComponentInfoController(this, m_instrWindow->getInstrumentActor(),m_selectionInfoDisplay);
    m_plotController = new DetectorPlotController(this, m_instrWindow->getInstrumentActor(),m_plot);
    m_plotController->setTubeXUnits( static_cast<DetectorPlotController::TubeXUnits>(m_tubeXUnitsCache) );
    m_plotController->setPlotType( static_cast<DetectorPlotController::PlotType>(m_plotTypeCache) );
    setSelectionType();
    setPlotCaption();
}

/**
  * Save tab's persistent settings to the provided QSettings instance
  */
void InstrumentWindowPickTab::saveSettings(QSettings &settings) const
{
  settings.setValue("TubeXUnits",m_plotController->getTubeXUnits());
  settings.setValue("PlotType", m_plotController->getPlotType());
}

/**
 * Restore (read and apply) tab's persistent settings from the provided QSettings instance
 */
void InstrumentWindowPickTab::loadSettings(const QSettings &settings)
{
  // loadSettings is called when m_plotController is not created yet.
  // Cache the settings and apply them later
  m_tubeXUnitsCache = settings.value("TubeXUnits",0).toInt();
  m_plotTypeCache =  settings.value("PlotType",DetectorPlotController::Single).toInt();
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


void InstrumentWindowPickTab::singleComponentTouched(size_t pickID)
{
  if (canUpdateTouchedDetector())
  {
    m_infoController->displayInfo( pickID );
    m_plotController->setPlotData( pickID );
    m_plotController->updatePlot();
  }
}

void InstrumentWindowPickTab::singleComponentPicked(size_t pickID)
{
    m_infoController->displayInfo( pickID );
    m_plotController->setPlotData( pickID );
    m_plotController->updatePlot();
}

/**
  * Update the selection display using currently selected detector.
  * Updates non-detector information on it.
  */
void InstrumentWindowPickTab::updateSelectionInfoDisplay()
{
    //updateSelectionInfo(m_currentDetID);
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
    if ( surface.hasMasks() )
    {
      QList<int> dets;
      surface.getMaskedDetectors( dets );
      m_plotController->setPlotData( dets );
    }
    else
    {
      m_plotController->clear();
    }
    m_plot->replot();
}

//=====================================================================================//

/**
 * Create and setup iteself.
 * @param parent :: QObject parent.
 * @param infoDisplay :: Widget on which to display the information.
 */
ComponentInfoController::ComponentInfoController(InstrumentWindowPickTab *tab, InstrumentActor* instrActor, QTextEdit* infoDisplay):
  QObject(tab),
  m_tab(tab),
  m_instrActor(instrActor),
  m_selectionInfoDisplay(infoDisplay),
  m_freezePlot(false),
  m_instrWindowBlocked(false),
  m_currentPickID(-1)
{
}

/**
 * Display info on a component refered to by a pick ID.
 * @param pickID :: A pick ID of a component.
 */
void ComponentInfoController::displayInfo(size_t pickID)
{
    if (m_freezePlot)
    {// freeze the plot for one update
      m_freezePlot = false;
      pickID = m_currentPickID;
    }
    int detid = m_instrActor->getDetID( pickID );
    displayDetectorInfo(detid);
}

/**
 * Display info on a detector.
 * @param detid :: A detector ID.
 */
void ComponentInfoController::displayDetectorInfo(Mantid::detid_t detid)
{
  if ( m_instrWindowBlocked ) 
  {
    m_selectionInfoDisplay->clear();
    return;
  }

  QString text;
  if (detid >= 0)
  {
    // collect info about selected detector and add it to text
    Mantid::Geometry::IDetector_const_sptr det;
    try
    {
        det = m_instrActor->getInstrument()->getDetector(detid);
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
      wsIndex = QString::number(m_instrActor->getWorkspaceIndex(detid));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      // Detector doesn't have a workspace index relating to it
      wsIndex = "None";
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
    const double integrated = m_instrActor->getIntegratedCounts(detid);
    const QString counts = integrated == -1.0 ? "N/A" : QString::number(integrated);
    text += "Counts: " + counts + '\n';
    //QString xUnits;
    //if (m_selectionType > SingleDetectorSelection && !m_plotSum)
    //{
    //  switch(m_tubeXUnits)
    //  {
    //  case DETECTOR_ID: xUnits = "Detector ID"; break;
    //  case LENGTH: xUnits = "Length"; break;
    //  case PHI: xUnits = "Phi"; break;
    //  case OUT_OF_PLANE_ANGLE: xUnits = "Out of plane angle"; break;
    //  default: xUnits = "Detector ID";
    //  }
    //}
    //else
    //{
    //  xUnits = QString::fromStdString(m_instrActor->getWorkspace()->getAxis(0)->unit()->caption());
    //}
    text += "X units: " + m_xUnits + '\n';
    // display info about peak overlays
    text += getParameterInfo(det);
  }
  else
  {
    //m_plot->clearCurve(); // Clear the plot window
    //m_plot->replot();
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
 * Form a string for output from the components instrument parameters
 */
QString ComponentInfoController::getParameterInfo(Mantid::Geometry::IComponent_const_sptr comp)
{  
  QString text = "";
  std::map<Mantid::Geometry::ComponentID, std::vector<std::string> > mapCmptToNameVector;

  auto paramNames = comp->getParameterNamesByComponent();
  for (auto itParamName = paramNames.begin(); itParamName != paramNames.end(); ++itParamName)
  {
    //build the data structure I need Map comp id -> vector of names
    std::string paramName = itParamName->first;
    Mantid::Geometry::ComponentID paramCompId = itParamName->second;
    //attempt to insert this will fail silently if the key already exists
    if ( mapCmptToNameVector.find(paramCompId) == mapCmptToNameVector.end() )
    {
      mapCmptToNameVector.insert(std::pair<Mantid::Geometry::ComponentID, std::vector<std::string> >(paramCompId,std::vector<std::string>()));
    }
    //get the vector out and add the name
    mapCmptToNameVector[paramCompId].push_back(paramName);
  }

  //walk out from the selected component
  Mantid::Geometry::IComponent_const_sptr paramComp = comp;
  while (paramComp)
  {
    auto& compParamNames = mapCmptToNameVector[paramComp->getComponentID()];
    if (compParamNames.size() > 0)
    {
      text += QString::fromStdString("\nParameters from: " + paramComp->getName() + "\n");
      std::sort(compParamNames.begin(), compParamNames.end(),Mantid::Kernel::CaseInsensitiveStringComparator());
      for (auto itParamName = compParamNames.begin(); itParamName != compParamNames.end(); ++itParamName)
      {
        std::string paramName = *itParamName;
        //no need to search recursively as we are asking from the matching component
        std::string paramValue = paramComp->getParameterAsString(paramName,false);
        if (paramValue != "")
        {
          text += QString::fromStdString(paramName + ": " + paramValue + "\n");
        }
      }
    }
    paramComp = paramComp->getParent();
  }

  return text;
}

/**
  * Return non-detector info to be displayed in the selection info display.
  */
QString ComponentInfoController::getNonDetectorInfo()
{
    QString text;
    QStringList overlays = m_tab->getSurface()->getPeaksWorkspaceNames();
    if ( !overlays.isEmpty() )
    {
        text += "Peaks:\n" + overlays.join("\n") + "\n";
    }
    return text;
}

//=====================================================================================//

/**
 * Constructor.
 * @param tab :: The parent tab.
 * @param instrActor :: A pointer to the InstrumentActor.
 * @param plot :: The plot widget.
 */
DetectorPlotController::DetectorPlotController(InstrumentWindowPickTab *tab, InstrumentActor* instrActor, OneCurvePlot* plot):
  QObject(tab),
  m_tab(tab),
  m_instrActor(instrActor),
  m_plot(plot),
  m_plotType(Single),
  m_enabled(true)
{
}

/**
 * Update the miniplot for a selected detector. The curve data depend on the 
 * plot type.
 * @param detid :: ID of detector to use to update the plot.
 */
void DetectorPlotController::setPlotData(size_t pickID)
{
  if ( m_plotType == DetectorSum )
  {
    m_plotType = Single;
  }

  int detid = m_instrActor->getDetID(pickID);

  if (!m_enabled)
  {
    m_plot->clearCurve();
    return;
  }

  if (detid >= 0)
  {
    if ( m_plotType == Single )
    {
      plotSingle(detid);
    }
    else if (m_plotType == TubeSum || m_plotType == TubeIntegral)
    {
      plotTube(detid);
    }
    else
    {
      throw std::logic_error("setPlotData: Unexpected plot type.");
    }
  }
  else
  {
    m_plot->clearCurve();
  }
}

/**
 * Set curev data from multiple detectors: sum their spectra.
 * @param detIDs :: A list of detector IDs.
 */
void DetectorPlotController::setPlotData(QList<int> detIDs)
{
  setPlotType( DetectorSum );
  clear();
  std::vector<double> x,y;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_instrActor->sumDetectors( detIDs, x, y, static_cast<size_t>(m_plot->width()) );
  QApplication::restoreOverrideCursor();
  if ( !x.empty() )
  {
      m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()), m_instrActor->getWorkspace()->getAxis(0)->unit()->unitID());
  }
  m_plot->setLabel("multiple");
}

/**
 * Update the miniplot for a selected detector.
 * @param detid :: ID of detector to use to update the plot.
 */
void DetectorPlotController::updatePlot()
{
  m_plot->recalcAxisDivs();
  m_plot->replot();
}

/**
 * Clear the plot.
 */
void DetectorPlotController::clear()
{
  m_plot->clearCurve();
  m_plot->clearPeakLabels();
}

/**
 * Plot data for a detector.
 * @param detid :: ID of the detector to be plotted.
 */
void DetectorPlotController::plotSingle(int detid)
{

  std::vector<double> x,y;
  prepareDataForSinglePlot(detid,x,y);

  m_plot->clearPeakLabels();
  // set the data 
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()), m_instrActor->getWorkspace()->getAxis(0)->unit()->unitID());
  m_plot->setLabel("Detector " + QString::number(detid));

  // find any markers
  auto surface = m_tab->getSurface();
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
void DetectorPlotController::plotTube(int detid)
{
  Mantid::API::MatrixWorkspace_const_sptr ws = m_instrActor->getWorkspace();
  Mantid::Geometry::IDetector_const_sptr det = m_instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  if (parent && ass)
  {
    if (m_plotType == TubeSum) // plot sums over detectors vs time bins
    {
      plotTubeSums(detid);
    }
    else // plot detector integrals vs detID or a function of detector position in the tube
    {
      assert( m_plotType == TubeIntegral );
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
void DetectorPlotController::plotTubeSums(int detid)
{
  std::vector<double> x,y;
  prepareDataForSumsPlot(detid,x,y);
  Mantid::Geometry::IDetector_const_sptr det = m_instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  QString label = QString::fromStdString(parent->getName()) + " (" + QString::number(detid) + ") Sum"; 
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()), m_instrActor->getWorkspace()->getAxis(0)->unit()->unitID());
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
void DetectorPlotController::plotTubeIntegrals(int detid)
{
  Mantid::Geometry::IDetector_const_sptr det = m_instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  // curve label: "tube_name (detid) Integrals"
  // detid is included to distiguish tubes with the same name
  QString label = QString::fromStdString(parent->getName()) + " (" + QString::number(detid) + ") Integrals"; 
  //label += "/" + getTubeXUnitsName(m_tubeXUnits);
  std::vector<double> x,y;
  prepareDataForIntegralsPlot(detid,x,y);
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()));
  m_plot->setLabel(label);
}

/**
 * Prepare data for plotting a spectrum of a single detector.
 * @param detid :: ID of the detector to be plotted.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void DetectorPlotController::prepareDataForSinglePlot(
  int detid,
  std::vector<double>&x,
  std::vector<double>&y,
  std::vector<double>* err)
{
  Mantid::API::MatrixWorkspace_const_sptr ws = m_instrActor->getWorkspace();
  size_t wi;
  try {
    wi = m_instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    return; // Detector doesn't have a workspace index relating to it
  }
  // get the data
  const Mantid::MantidVec& X = ws->readX(wi);
  const Mantid::MantidVec& Y = ws->readY(wi);
  const Mantid::MantidVec& E = ws->readE(wi);

  // find min and max for x
  size_t imin,imax;
  m_instrActor->getBinMinMaxIndex(wi,imin,imax);

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
void DetectorPlotController::prepareDataForSumsPlot(
  int detid,
  std::vector<double>&x,
  std::vector<double>&y,
  std::vector<double>* err)
{
  Mantid::API::MatrixWorkspace_const_sptr ws = m_instrActor->getWorkspace();
  Mantid::Geometry::IDetector_const_sptr det = m_instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  size_t wi;
  try {
    wi = m_instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    return; // Detector doesn't have a workspace index relating to it
  }
  size_t imin,imax;
  m_instrActor->getBinMinMaxIndex(wi,imin,imax);

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
        size_t index = m_instrActor->getWorkspaceIndex(idet->getID());
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
 *   OUT_OF_PLANE_ANGLE
 * The units can be set with setTubeXUnits(...) method.
 * @param detid :: A detector id. The miniplot will display data for a component containing the detector 
 *   with this id.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void DetectorPlotController::prepareDataForIntegralsPlot(
  int detid,
  std::vector<double>&x,
  std::vector<double>&y,
  std::vector<double>* err)
{
  Mantid::API::MatrixWorkspace_const_sptr ws = m_instrActor->getWorkspace();

  // Does the instrument definition specify that psi should be offset.
  std::vector<std::string> parameters = ws->getInstrument()->getStringParameter("offset-phi");
  const bool bOffsetPsi = (!parameters.empty()) && std::find(parameters.begin(), parameters.end(), "Always") != parameters.end();

  Mantid::Geometry::IDetector_const_sptr det = m_instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  size_t wi;
  try {
    wi = m_instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    return; // Detector doesn't have a workspace index relating to it
  }
  // imin and imax give the bin integration range
  size_t imin,imax;
  m_instrActor->getBinMinMaxIndex(wi,imin,imax);

  Mantid::Kernel::V3D samplePos = m_instrActor->getInstrument()->getSample()->getPos();

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
  Mantid::Kernel::V3D normal = (*ass)[1]->getPos() - idet0->getPos();
  normal.normalize();
  for(int i = 0; i < n; ++i)
  {
    Mantid::Geometry::IDetector_sptr idet = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*ass)[i]);
    if (idet)
    {
      try {
        const int id = idet->getID();
        // get the x-value for detector idet
        double xvalue = 0;
        switch(m_tubeXUnits)
        {
        case LENGTH: xvalue = idet->getDistance(*idet0); break;
        case PHI: xvalue = bOffsetPsi ? idet->getPhiOffset(M_PI) : idet->getPhi(); break;
        case OUT_OF_PLANE_ANGLE: 
          {
            Mantid::Kernel::V3D pos = idet->getPos();
            xvalue = getOutOfPlaneAngle(pos, samplePos, normal);
            break;
          }
        default: xvalue = static_cast<double>(id);
        }
        size_t index = m_instrActor->getWorkspaceIndex(id);
        // get the y-value for detector idet
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
 * Save data plotted on the miniplot into a MatrixWorkspace.
 */
void DetectorPlotController::savePlotToWorkspace()
{
  if (!m_plot->hasCurve() && !m_plot->hasStored())
  {
    // nothing to save
    return;
  }
  Mantid::API::MatrixWorkspace_const_sptr parentWorkspace = m_instrActor->getWorkspace();
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
      if ( X.empty() )
      {
        // label doesn't have any info on how to reproduce the curve:
        // only the current curve can be saved
        QList<int> dets;
        m_tab->getSurface()->getMaskedDetectors( dets );
        m_instrActor->sumDetectors( dets, x, y );
        unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      }
      else
      {
        QMessageBox::warning(NULL,"MantidPlot - Warning","Cannot save the stored curves.\nOnly the current curve will be saved.");
      }
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
    if (!x.empty())
    {
      if (nbins > 0 && x.size() != nbins)
      {
        QMessageBox::critical(NULL,"MantidPlot - Error","Curves have different sizes.");
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
    E.resize(Y.size(),1.0);
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
  * Calculate the angle between a vector ( == pos - origin ) and a plane ( orthogonal to normal ).
  * The angle is positive if the vector and the normal make an acute angle.
  * @param pos :: Vector's end.
  * @param origin :: Vector's origin.
  * @param normal :: Normal to the plane.
  * @return :: Angle between the vector and the plane in radians in [-pi/2, pi/2]. 
  */
double DetectorPlotController::getOutOfPlaneAngle(const Mantid::Kernel::V3D& pos, const Mantid::Kernel::V3D& origin, const Mantid::Kernel::V3D& normal)
{
  Mantid::Kernel::V3D vec = pos - origin;
  vec.normalize();
  return asin(vec.scalar_prod(normal));
}

/**
 * Return symbolic name of a TubeXUnit.
 * @param unit :: One of TubeXUnits.
 * @return :: Symbolic name of the units, caseless: Detector_ID, Length, Phi
 */
QString DetectorPlotController::getTubeXUnitsName(DetectorPlotController::TubeXUnits unit) const
{
  switch(unit)
  {
  case LENGTH: return "Length";
  case PHI: return "Phi";
  case OUT_OF_PLANE_ANGLE: return "Out of plane angle";
  default: return "Detector_ID";
  }
  return "Detector_ID";
}

/**
 * Get the plot caption for the current plot type.
 */
QString DetectorPlotController::getPlotCaption() const
{

  switch( m_plotType )
  {
  case Single:      return "Plotting detector spectra";
  case DetectorSum: return "Plotting multiple detector sum";
  case TubeSum:     return "Plotting sum";
  case TubeIntegral:return "Plotting integral";
  default: throw std::logic_error("getPlotCaption: Unknown plot type.");
  }

}

