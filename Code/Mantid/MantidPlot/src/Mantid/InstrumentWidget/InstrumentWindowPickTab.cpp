#include "InstrumentWindow.h"
#include "InstrumentWindowPickTab.h"
#include "OneCurvePlot.h"
#include "CollapsiblePanel.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"
#include "PeakMarker2D.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"

#include "qwt_scale_widget.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QMessageBox>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QSignalMapper>

#include <numeric>
#include <cfloat>
#include <cmath>
#include <algorithm>

class InputConvertUnitsParametersDialog : public QDialog
{
public:
  InputConvertUnitsParametersDialog(QWidget* parent);
  int getEMode()const;
  double getEFixed()const;
  double getDelta()const;
private:
  QComboBox* m_emode;
  QLineEdit* m_efixed;
  QLineEdit* m_delta;
};

InputConvertUnitsParametersDialog::InputConvertUnitsParametersDialog(QWidget* parent):QDialog(parent)
{
  QGridLayout* input_layout = new QGridLayout();
  QLabel* label = new QLabel("Units have to be converted to TOF.\nPlease specify additional information.");
  m_emode = new QComboBox();
  QStringList emodeOptions;
  emodeOptions << "Elastic" << "Direct" << "Indirect";
  m_emode->insertItems(0,emodeOptions);
  QLabel* emode_label = new QLabel("EMode");
  
  m_efixed = new QLineEdit();
  m_efixed->setText("0.0");
  QLabel* efixed_label = new QLabel("EFixed");

  m_delta = new QLineEdit();
  m_delta->setText("0.0");
  QLabel* delta_label = new QLabel("Delta");

  input_layout->addWidget(label,0,0,1,2);
  input_layout->addWidget(emode_label,1,0);
  input_layout->addWidget(m_emode,1,1);
  input_layout->addWidget(efixed_label,2,0);
  input_layout->addWidget(m_efixed,2,1);
  input_layout->addWidget(delta_label,3,0);
  input_layout->addWidget(m_delta,3,1);

  QHBoxLayout* button_layout = new QHBoxLayout();
  QPushButton* ok_button = new QPushButton("OK");
  button_layout->addStretch();
  button_layout->addWidget(ok_button);
  connect(ok_button,SIGNAL(clicked()),this,SLOT(close()));

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->addLayout(input_layout);
  main_layout->addStretch();
  main_layout->addLayout(button_layout);
}

int InputConvertUnitsParametersDialog::getEMode()const
{
  return m_emode->currentIndex();
}

double InputConvertUnitsParametersDialog::getEFixed()const
{
  return m_efixed->text().toDouble();
}

double InputConvertUnitsParametersDialog::getDelta()const
{
  return m_delta->text().toDouble();
}

// --- InstrumentWindowPickTab --- //

InstrumentWindowPickTab::InstrumentWindowPickTab(InstrumentWindow* instrWindow):
QFrame(instrWindow),m_instrWindow(instrWindow),m_currentDetID(-1)
{
  mInstrumentDisplay = m_instrWindow->getInstrumentDisplay();
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
  m_integrateTimeBins = new QAction("Integrate",this);
  m_logY = new QAction("Y log scale",this);
  m_linearY = new QAction("Y linear scale",this);
  connect(m_sumDetectors,SIGNAL(triggered()),this,SLOT(sumDetectors()));
  connect(m_integrateTimeBins,SIGNAL(triggered()),this,SLOT(integrateTimeBins()));
  connect(m_logY,SIGNAL(triggered()),m_plot,SLOT(setYLogScale()));
  connect(m_linearY,SIGNAL(triggered()),m_plot,SLOT(setYLinearScale()));

  // Instrument display context menu actions
  m_storeCurve = new QAction("Store curve",this);
  connect(m_storeCurve,SIGNAL(triggered()),this,SLOT(storeCurve()));

  CollapsibleStack* panelStack = new CollapsibleStack(this);
  m_infoPanel = panelStack->addPanel("Selection",m_selectionInfoDisplay);
  m_plotPanel = panelStack->addPanel("Name",m_plot);

  m_activeTool = new QLabel(this);
  // set up the tool bar
  m_one = new QPushButton();
  m_one->setCheckable(true);
  m_one->setAutoExclusive(true);
  m_one->setChecked(true);
  m_one->setToolTip("Select single pixel");
  m_one->setIcon(QIcon(":/PickTools/selection-pointer.png"));

  m_box = new QPushButton();
  m_box->setCheckable(true);
  m_box->setAutoExclusive(true);
  m_box->setIcon(QIcon(":/PickTools/selection-box.png"));

  m_tube = new QPushButton();
  m_tube->setCheckable(true);
  m_tube->setAutoExclusive(true);
  m_tube->setIcon(QIcon(":/PickTools/selection-tube.png"));
  m_tube->setToolTip("Select whole tube");

  m_peak = new QPushButton();
  m_peak->setCheckable(true);
  m_peak->setAutoExclusive(true);
  m_peak->setIcon(QIcon(":/PickTools/selection-peak.png"));
  m_peak->setToolTip("Select single crystal peak");

  QHBoxLayout* toolBox = new QHBoxLayout();
  toolBox->addWidget(m_one);
  toolBox->addWidget(m_box); 
  m_box->setVisible(false); //Hidden by Owen Arnold 14/02/2011 because box picking doesn't exhibit correct behaviour and is not necessary for current release 
  toolBox->addWidget(m_tube);
  toolBox->addWidget(m_peak);
  toolBox->addStretch();
  toolBox->setSpacing(2);
  connect(m_one,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_box,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_tube,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  connect(m_peak,SIGNAL(clicked()),this,SLOT(setSelectionType()));
  setSelectionType();

  // lay out the widgets
  layout->addWidget(m_activeTool);
  layout->addLayout(toolBox);
  layout->addWidget(panelStack);

  setPlotCaption();
}

void InstrumentWindowPickTab::init()
{
  m_emode = -1;
}

/**
  * Returns true if the plot can be updated when the mouse moves over detectors
  */
bool InstrumentWindowPickTab::canUpdateTouchedDetector()const
{
  return ! m_peak->isChecked();
}

void InstrumentWindowPickTab::updatePlot(int detid)
{
  if (m_instrWindow->blocked())
  {
    m_plot->clearCurve();
    return;
  }
  if (m_plotPanel->isCollapsed()) return;
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
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

void InstrumentWindowPickTab::updateSelectionInfo(int detid)
{
  if (m_instrWindow->blocked()) 
  {
    m_selectionInfoDisplay->clear();
    return;
  }
  if (detid >= 0)
  {
    InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
    Mantid::Geometry::IDetector_const_sptr det = instrActor->getInstrument()->getDetector(detid);
    QString text = "Selected detector: " + QString::fromStdString(det->getName()) + "\n";
    text += "Detector ID: " + QString::number(detid) + '\n';
    QString wsIndex;
    try {
      wsIndex = QString::number(instrActor->getWorkspaceIndex(detid));
      updatePlot(detid); // Update the plot if the detector links to some data
    } catch (Mantid::Kernel::Exception::NotFoundError) {
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
    m_selectionInfoDisplay->setText(text);
  }
  else
  {
    m_selectionInfoDisplay->clear();
    m_plot->clearCurve(); // Clear the plot window
    m_plot->replot();
  }
}

void InstrumentWindowPickTab::plotContextMenu()
{
  QMenu context(this);
  
  if (m_selectionType > SingleDetectorSelection)
  {// only for multiple detector selectors
    context.addAction(m_sumDetectors);
    context.addAction(m_integrateTimeBins);
  }

  if (m_plot->hasStored())
  {
    QMenu *removeCurves = new QMenu("Remove",this);
    QSignalMapper *signalMapper = new QSignalMapper(this);
    QStringList labels = m_plot->getLabels();
    foreach(QString label,labels)
    {
      QAction *remove = new QAction(label,removeCurves);
      removeCurves->addAction(remove);
      connect(remove,SIGNAL(triggered()),signalMapper,SLOT(map()));
      signalMapper->setMapping(remove,label);
    }
    connect(signalMapper, SIGNAL(mapped(const QString &)),
             this, SLOT(removeCurve(const QString &)));
    context.addMenu(removeCurves);
  }

  QMenu* axes = new QMenu("Axes",this);
  axes->addAction(m_logY);
  axes->addAction(m_linearY);
  context.addMenu(axes);

  context.exec(QCursor::pos());
}

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

void InstrumentWindowPickTab::sumDetectors()
{
  m_plotSum = true;
  setPlotCaption();
}

void InstrumentWindowPickTab::integrateTimeBins()
{
  m_plotSum = false;
  setPlotCaption();
}

void InstrumentWindowPickTab::updatePick(int detid)
{
  updateSelectionInfo(detid); // Also calls updatePlot
  m_currentDetID = detid;
}

void InstrumentWindowPickTab::getBinMinMaxIndex(size_t wi,size_t& imin, size_t& imax)
{
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
  const Mantid::MantidVec& x = ws->readX(wi);
  if (instrActor->wholeRange())
  {
    imin = 0;
    imax = x.size() - 1;
  }
  else
  {
    Mantid::MantidVec::const_iterator x_begin = std::lower_bound(x.begin(),x.end(),instrActor->minBinValue());
    Mantid::MantidVec::const_iterator x_end = std::lower_bound(x.begin(),x.end(),instrActor->maxBinValue());
    imin = static_cast<size_t>(x_begin - x.begin());
    imax = static_cast<size_t>(x_end - x.begin()) - 1;
  }
}

/**
 * Plot data for a detector.
 * @param detid :: ID of the detector to be plotted.
 */
void InstrumentWindowPickTab::plotSingle(int detid)
{
  m_plot->clearPeakLabels();
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
  size_t wi;
  try {
    wi = instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError) {
    return; // Detector doesn't have a workspace index relating to it
  }
  // get the data
  const Mantid::MantidVec& x = ws->readX(wi);
  const Mantid::MantidVec& y = ws->readY(wi);

  // find min and max for x
  size_t imin,imax;
  getBinMinMaxIndex(wi,imin,imax);

  Mantid::MantidVec::const_iterator y_begin = y.begin() + imin;
  Mantid::MantidVec::const_iterator y_end = y.begin() + imax;

  m_plot->setXScale(x[imin],x[imax]);

  // fins min and max for y
  Mantid::MantidVec::const_iterator min_it = std::min_element(y_begin,y_end);
  Mantid::MantidVec::const_iterator max_it = std::max_element(y_begin,y_end);
  // set the data 
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()));
  m_plot->setYScale(*min_it,*max_it);
  m_plot->setLabel("Detector " + QString::number(detid));

  // find any markers
  ProjectionSurface* surface = mInstrumentDisplay->getSurface();
  if (surface)
  {
    QList<PeakMarker2D*> markers = surface->getMarkersWithID(detid);
    foreach(PeakMarker2D* marker,markers)
    {
      m_plot->addPeakLabel(new PeakLabel(marker));
      //std::cerr << marker->getLabel().toStdString() << std::endl;
    }
  }
}

void InstrumentWindowPickTab::plotTube(int detid)
{
  InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
  size_t wi;
  try {
    wi = instrActor->getWorkspaceIndex(detid);
  } catch (Mantid::Kernel::Exception::NotFoundError) {
    return; // Detector doesn't have a workspace index relating to it
  }
  Mantid::Geometry::IDetector_const_sptr det = instrActor->getInstrument()->getDetector(detid);
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  if (parent && ass)
  {
    size_t imin,imax;
    getBinMinMaxIndex(wi,imin,imax);

    const int n = ass->nelements();
    if (m_plotSum) // plot sums over detectors vs time bins
    {
      const Mantid::MantidVec& x = ws->readX(wi);

      m_plot->setXScale(x[imin],x[imax]);

      std::vector<double> y(ws->blocksize());
      //std::cerr<<"plotting sum of " << ass->nelements() << " detectors\n";
      for(int i = 0; i < n; ++i)
      {
        Mantid::Geometry::IDetector_sptr idet = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*ass)[i]);
        if (idet)
        {
          try {
            size_t index = instrActor->getWorkspaceIndex(idet->getID());
            const Mantid::MantidVec& Y = ws->readY(index);
            std::transform(y.begin(),y.end(),Y.begin(),y.begin(),std::plus<double>());
          } catch (Mantid::Kernel::Exception::NotFoundError) {
            continue; // Detector doesn't have a workspace index relating to it
          }
        }
      }
      Mantid::MantidVec::const_iterator y_begin = y.begin() + imin;
      Mantid::MantidVec::const_iterator y_end = y.begin() + imax;

      Mantid::MantidVec::const_iterator min_it = std::min_element(y_begin,y_end);
      Mantid::MantidVec::const_iterator max_it = std::max_element(y_begin,y_end);
      m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()));
      m_plot->setYScale(*min_it,*max_it);
    }
    else // plot detector integrals vs detID
    {
      std::vector<double> x;
      x.reserve(n);
      std::map<double,double> ymap;
      for(int i = 0; i < n; ++i)
      {
        Mantid::Geometry::IDetector_sptr idet = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*ass)[i]);
        if (idet)
        {
          try {
            const int id = idet->getID();
            size_t index = instrActor->getWorkspaceIndex(id);
            x.push_back(id);
            const Mantid::MantidVec& Y = ws->readY(index);
            double sum = std::accumulate(Y.begin() + imin,Y.begin() + imax,0);
            ymap[id] = sum;
          } catch (Mantid::Kernel::Exception::NotFoundError) {
            continue; // Detector doesn't have a workspace index relating to it
          }
        }
      }
      if (!x.empty())
      {
        std::sort(x.begin(),x.end());
        std::vector<double> y(x.size());
        double ymin =  DBL_MAX;
        double ymax = -DBL_MAX;
        for(size_t i = 0; i < x.size(); ++i)
        {
          const double val = ymap[x[i]];
          y[i] = val;
          if (val < ymin) ymin = val;
          if (val > ymax) ymax = val;
        }
        m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()));
        m_plot->setXScale(x.front(),x.back());
        m_plot->setYScale(ymin,ymax);
      }
    }
  }
  else
  {
    m_plot->clearCurve();
  }
}

void InstrumentWindowPickTab::setSelectionType()
{
  if (m_one->isChecked())
  {
    m_selectionType = Single;
    m_activeTool->setText("Tool: Pixel selection");
  }
  else if (m_box->isChecked())
  {
    m_selectionType = BoxType;
    m_activeTool->setText("Tool: Box selection");
  }
  else if (m_tube->isChecked())
  {
    m_selectionType = Tube;
    m_activeTool->setText("Tool: Tube/bank selection");
  }
  else if (m_peak->isChecked())
  {
    m_selectionType = Peak;
    m_activeTool->setText("Tool: Single crystal peak selection");
  }
  setPlotCaption();
  //mInstrumentDisplay->setSelectionType(m_selectionType);
}

/**
  * Add a peak to the single crystal peak table.
  * @param x :: Time of flight
  * @param y :: Peak height (counts)
  */
void InstrumentWindowPickTab::addPeak(double x,double y)
{
  UNUSED_ARG(y)
  if (!m_peak->isChecked() ||  m_currentDetID < 0) return;
  Mantid::API::IPeaksWorkspace_sptr tw;
  std::string peakTableName = "SingleCrystalPeakTable";

  try
  {
    const double mN =   1.67492729e-27;
    const double hbar = 1.054571628e-34;

    InstrumentActor* instrActor = m_instrWindow->getInstrumentActor();
    Mantid::API::MatrixWorkspace_const_sptr ws = instrActor->getWorkspace();
    // This does need to get the instrument from the workspace as it's doing calculations
    // .....and this method should be an algorithm! Or at least somewhere different to here.
    Mantid::Geometry::Instrument_const_sptr instr = ws->getInstrument();
    Mantid::Geometry::IObjComponent_const_sptr source = instr->getSource();
    Mantid::Geometry::IObjComponent_const_sptr sample = instr->getSample();
    Mantid::Geometry::IDetector_const_sptr det = instr->getDetector(m_currentDetID);

    Mantid::Kernel::Unit_sptr unit = ws->getAxis(0)->unit();

    if (! Mantid::API::AnalysisDataService::Instance().doesExist(peakTableName))
    {
      tw = Mantid::API::WorkspaceFactory::Instance().createPeaks("PeaksWorkspace");
      tw->setInstrument(instr);
      //tw->addColumn("double","Qx");
      //tw->addColumn("double","Qy");
      //tw->addColumn("double","Qz");
      Mantid::API::AnalysisDataService::Instance().add(peakTableName,tw);
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
    const Mantid::Kernel::V3D samplePos = sample->getPos();
    const Mantid::Kernel::V3D beamLine = samplePos - source->getPos();
    double theta2 = det->getTwoTheta(samplePos,beamLine);
    double phi = det->getPhi();

    // In the inelastic convention, Q = ki - kf.
    double Qx=-sin(theta2)*cos(phi);
    double Qy=-sin(theta2)*sin(phi);
    double Qz=1.0-cos(theta2);
    double l1 = source->getDistance(*sample);
    double l2 = det->getDistance(*sample);

    double tof = x;
    if (unit->unitID() != "TOF")
    {
      if (m_emode < 0)
      {
        const Mantid::API::Run & run = ws->run();
        if (run.hasProperty("Ei"))
        {
          m_emode = 1; // direct
        }
        else if (det->hasParameter("Efixed"))
        {
          m_emode = 2; // indirect
        }
        else
        {
          //m_emode = 0; // Elastic
          InputConvertUnitsParametersDialog* dlg = new InputConvertUnitsParametersDialog(this);
          dlg->exec();
          m_emode = dlg->getEMode();
          m_efixed = dlg->getEFixed();
          m_delta = dlg->getDelta();
        }
      }
      std::vector<double> xdata(1,x);
      std::vector<double> ydata;
      unit->toTOF(xdata, ydata, l1, l2, theta2, m_emode, m_efixed, m_delta);
      tof = xdata[0];
    }

    double knorm=mN*(l1 + l2)/(hbar*tof*1e-6)/1e10;
    knorm/=(2.*M_PI); //Peak constructor uses Q/(2*Pi)
    Qx *= knorm;
    Qy *= knorm;
    Qz *= knorm;

    Mantid::API::IPeak* peak = tw->createPeak(Mantid::Kernel::V3D(Qx,Qy,Qz),l2);
    peak->setDetectorID(m_currentDetID);
    tw->addPeak(*peak);
    delete peak;
    tw->modified();
  }
  catch(std::exception& e)
  {
    if (tw)
    {
      Mantid::API::AnalysisDataService::Instance().remove(peakTableName);
    }
    QMessageBox::critical(this,"MantidPlot -Error",
      "Cannot create a Peak object because of the error:\n"+QString(e.what()));
  }

}

void InstrumentWindowPickTab::showEvent (QShowEvent *)
{
  ProjectionSurface* surface = mInstrumentDisplay->getSurface();
  if (surface)
  {
    surface->setInteractionModePick();
  }
  mInstrumentDisplay->setMouseTracking(true);
}

/**
 * Show context menu of mInstrumentDisplay
 */
void InstrumentWindowPickTab::showInstrumentDisplayContextMenu()
{
  if (m_plot->hasCurve())
  {
    QMenu context(this);
    context.addAction(m_storeCurve);
    context.exec(QCursor::pos());
  }
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
