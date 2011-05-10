#include "InstrumentWindow.h"
#include "InstrumentWindowPickTab.h"
#include "OneCurvePlot.h"
#include "CollapsiblePanel.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
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

#include <numeric>
#include <cfloat>

InstrumentWindowPickTab::InstrumentWindowPickTab(InstrumentWindow* instrWindow):
QFrame(instrWindow),m_instrWindow(instrWindow)
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

  m_sumDetectors = new QAction("Sum",this);
  m_integrateTimeBins = new QAction("Integrate",this);
  m_logY = new QAction("Y log scale",this);
  m_linearY = new QAction("Y linear scale",this);
  connect(m_sumDetectors,SIGNAL(triggered()),this,SLOT(sumDetectors()));
  connect(m_integrateTimeBins,SIGNAL(triggered()),this,SLOT(integrateTimeBins()));
  connect(m_logY,SIGNAL(triggered()),m_plot,SLOT(setYLogScale()));
  connect(m_linearY,SIGNAL(triggered()),m_plot,SLOT(setYLinearScale()));

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

/**
  * Returns true if the plot can be updated when the mouse moves over detectors
  */
bool InstrumentWindowPickTab::canUpdateTouchedDetector()const
{
  return ! m_peak->isChecked();
}

void InstrumentWindowPickTab::updatePlot(const Instrument3DWidget::DetInfo & cursorPos)
{
  if (m_instrWindow->blocked())
  {
    m_plot->clearCurve();
    return;
  }
  if (m_plotPanel->isCollapsed()) return;
  Mantid::API::MatrixWorkspace_const_sptr ws = cursorPos.getWorkspace();
  int wi = cursorPos.getWorkspaceIndex();
  if (cursorPos.getDetID() >= 0 && wi >= 0)
  {
    if (m_one->isChecked() || m_peak->isChecked())
    {// plot spectrum of a single detector
      plotSingle(cursorPos);
    }
    else if (m_tube->isChecked())
    {// plot integrals
      plotTube(cursorPos);
    }
  }
  else
  {
    m_plot->clearCurve();
  }
  m_plot->recalcAxisDivs();
  m_plot->replot();
}

void InstrumentWindowPickTab::updateSelectionInfo(const Instrument3DWidget::DetInfo & cursorPos)
{
  if (m_instrWindow->blocked()) 
  {
    m_selectionInfoDisplay->clear();
    return;
  }
  if (cursorPos.getDetID() >= 0)
  {
    Mantid::API::MatrixWorkspace_const_sptr ws = cursorPos.getWorkspace();
    Mantid::Geometry::IDetector_sptr det = ws->getInstrument()->getDetector(cursorPos.getDetID());
    QString text = "Selected detector: " + QString::fromStdString(det->getName()) + "\n";
    text += "Detector ID: " + QString::number(cursorPos.getDetID()) + '\n';
    text += "Workspace index: " + QString::number(cursorPos.getWorkspaceIndex()) + '\n';
    Mantid::Geometry::V3D pos = det->getPos();
    text += "xyz: " + QString::number(pos.X()) + "," + QString::number(pos.Y()) + "," + QString::number(pos.Z())  + '\n';
    double r,t,p;
    pos.getSpherical(r,t,p);
    text += "rtp: " + QString::number(r) + "," + QString::number(t) + "," + QString::number(p)  + '\n';
    Mantid::Geometry::ICompAssembly_const_sptr parent = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(det->getParent());
    if (parent)
    {
      text += "Parent assembly: " + QString::fromStdString(parent->getName()) + '\n';
    }
    m_selectionInfoDisplay->setText(text);
  }
  else
  {
    m_selectionInfoDisplay->clear();
  }
}

void InstrumentWindowPickTab::plotContextMenu()
{
  QMenu context(this);
  
  context.addAction(m_sumDetectors);
  context.addAction(m_integrateTimeBins);

  QMenu* axes = new QMenu("Axes",this);
  axes->addAction(m_logY);
  axes->addAction(m_linearY);
  context.addMenu(axes);

  context.exec(QCursor::pos());
}

void InstrumentWindowPickTab::setPlotCaption()
{
  QString caption;
  if (m_selectionType == Single)
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

void InstrumentWindowPickTab::updatePick(const Instrument3DWidget::DetInfo & cursorPos)
{
  m_currentPos = cursorPos;
  updatePlot(cursorPos);
  updateSelectionInfo(cursorPos);
}

void InstrumentWindowPickTab::plotSingle(const Instrument3DWidget::DetInfo & cursorPos)
{
  Mantid::API::MatrixWorkspace_const_sptr ws = cursorPos.getWorkspace();
  int wi = cursorPos.getWorkspaceIndex();
  const Mantid::MantidVec& x = ws->readX(wi);
  const Mantid::MantidVec& y = ws->readY(wi);
  m_plot->setXScale(x.front(),x.back());
  Mantid::MantidVec::const_iterator min_it = std::min_element(y.begin(),y.end());
  Mantid::MantidVec::const_iterator max_it = std::max_element(y.begin(),y.end());
  m_plot->setData(&x[0],&y[0],static_cast<int>(y.size()));
  m_plot->setYScale(*min_it,*max_it);
}

void InstrumentWindowPickTab::plotBox(const Instrument3DWidget::DetInfo & /*cursorPos*/)
{
}

void InstrumentWindowPickTab::plotTube(const Instrument3DWidget::DetInfo & cursorPos)
{
  Mantid::API::MatrixWorkspace_const_sptr ws = cursorPos.getWorkspace();
  int wi = cursorPos.getWorkspaceIndex();
  Mantid::Geometry::IDetector_sptr det = ws->getInstrument()->getDetector(cursorPos.getDetID());
  boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
  Mantid::Geometry::ICompAssembly_const_sptr ass = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(parent);
  if (parent && ass)
  {
    const int n = ass->nelements();
    if (m_plotSum) // plot sums over detectors vs time bins
    {
      const Mantid::MantidVec& x = ws->readX(wi);
      m_plot->setXScale(x.front(),x.back());
      std::vector<double> y(ws->blocksize());
      //std::cerr<<"plotting sum of " << ass->nelements() << " detectors\n";
      for(int i = 0; i < n; ++i)
      {
        Mantid::Geometry::IDetector_sptr idet = boost::dynamic_pointer_cast<Mantid::Geometry::IDetector>((*ass)[i]);
        if (idet)
        {
          int index = cursorPos.getIndexOf(idet->getID());
          if (index >= 0)
          {
            const Mantid::MantidVec& Y = ws->readY(index);
            std::transform(y.begin(),y.end(),Y.begin(),y.begin(),std::plus<double>());
          }
        }
      }
      Mantid::MantidVec::const_iterator min_it = std::min_element(y.begin(),y.end());
      Mantid::MantidVec::const_iterator max_it = std::max_element(y.begin(),y.end());
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
          const int id = idet->getID();
          int index = cursorPos.getIndexOf(id);
          if (index >= 0)
          {
            x.push_back(id);
            const Mantid::MantidVec& Y = ws->readY(index);
            double sum = std::accumulate(Y.begin(),Y.end(),0);
            ymap[id] = sum;
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
  mInstrumentDisplay->setSelectionType(m_selectionType);
}

/**
  * Add a peak to the single crystal peak table.
  * @param x :: Time of flight
  * @param y :: Peak height (counts)
  */
void InstrumentWindowPickTab::addPeak(double x,double y)
{
  if (!m_peak->isChecked() ||  m_currentPos.getDetID() < 0) return;
  std::string peakTableName = "SingleCrystalPeakTable";
  Mantid::API::ITableWorkspace_sptr tw;
  if (! Mantid::API::AnalysisDataService::Instance().doesExist(peakTableName))
  {
    tw = Mantid::API::WorkspaceFactory::Instance().createTable();
    tw->addColumn("double","Qx");
    tw->addColumn("double","Qy");
    tw->addColumn("double","Qz");
    Mantid::API::AnalysisDataService::Instance().add(peakTableName,tw);
  }
  else
  {
    tw = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(peakTableName));
    if (!tw)
    {
      QMessageBox::critical(this,"Mantid - Error","Workspace " + QString::fromStdString(peakTableName) + " is not a TableWorkspace");
      return;
    }
  }
  const double mN =   1.67492729e-27;
  const double hbar = 1.054571628e-34;
  
  Mantid::Geometry::IInstrument_const_sptr instr = m_currentPos.getWorkspace()->getInstrument();
  Mantid::Geometry::IObjComponent_const_sptr source = instr->getSource();
  Mantid::Geometry::IObjComponent_const_sptr sample = instr->getSample();
  Mantid::Geometry::IDetector_const_sptr det = instr->getDetector(m_currentPos.getDetID());

  const Mantid::Geometry::V3D samplePos = sample->getPos();
  const Mantid::Geometry::V3D beamLine = samplePos - source->getPos();
  double theta2 = det->getTwoTheta(samplePos,beamLine);
  double phi = det->getPhi();

  double Qx=sin(theta2)*cos(phi);
  double Qy=sin(theta2)*sin(phi);
  double Qz=cos(theta2)-1.0;
  double knorm=mN*(source->getDistance(*sample) + det->getDistance(*sample))/(hbar*x*1e-6)/1e10;
  Qx *= knorm;
  Qy *= knorm;
  Qz *= knorm;

  Mantid::API::TableRow row = tw->appendRow();
  row << Qx << Qy << Qz;

  //std::cerr << "id=" << det->getID() << std::endl;
  //std::cerr << "l1=" << source->getDistance(*sample) << " l2=" << det->getDistance(*sample) << std::endl;
  //std::cerr << "2th=" << theta2/M_PI*180 << " phi=" << phi/M_PI*180 << std::endl;
}
