#include "MantidQtCustomInterfaces/C2ETab.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  C2ETab::C2ETab(Ui::ConvertToEnergy& uiForm, QWidget * parent) : QWidget(parent),
      m_plot(new QwtPlot(parent)), m_curve(new QwtPlotCurve()), m_rangeSelector(new MantidWidgets::RangeSelector(m_plot)),
      m_propTree(new QtTreePropertyBrowser()), m_properties(), m_dblManager(new QtDoublePropertyManager()), 
      m_dblEdFac(new DoubleEditorFactory()), m_algRunner(new MantidQt::API::AlgorithmRunner(this)), m_uiForm(uiForm)
  {
    QObject::connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(algorithmFinished(bool)));
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  C2ETab::~C2ETab()
  {
  }
  
  void C2ETab::runTab()
  {
    if(validate())
    {
      run();
    }
  }

  void C2ETab::setupTab()
  {
    setup();
  }

  void C2ETab::validateTab()
  {
    validate();
  }

  /**
   * Run the load algorithm with the supplied filename
   * 
   * @param filename :: The name of the file to load
   * @param outputName :: The name of the output workspace
   * @return If the algorithm was successful
   */
  bool C2ETab::loadFile(const QString& filename, const QString& outputName)
  {
    using namespace Mantid::API;

    Algorithm_sptr load = AlgorithmManager::Instance().createUnmanaged("Load", -1);
    load->initialize();
    load->setProperty("Filename", filename.toStdString());
    load->setProperty("OutputWorkspace", outputName.toStdString());
    load->execute();
    
    //if reloading fails we're out of options
    if(!load->isExecuted())
    {
      return false;
    }

    return true;
  }

  /**
   * Plot a workspace to the miniplot given a workspace name and
   * a specturm index.
   *
   * This method uses the analysis data service to retrieve the workspace.
   * 
   * @param workspace :: The name of the workspace
   * @param index :: The spectrum index of the workspace
   */
  void C2ETab::plotMiniPlot(const QString& workspace, size_t index)
  {
    using namespace Mantid::API;
    auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(workspace.toStdString());
    plotMiniPlot(ws, index);
  }

  /**
   * Gets the range of the curve plotted in the mini plot
   *
   * @return A pair containing the maximum and minimum points of the curve
   */
  std::pair<double,double> C2ETab::getCurveRange()
  {
    size_t npts = m_curve->data().size();

    if( npts < 2 )
      throw std::invalid_argument("Too few points on data curve to determine range.");

    return std::make_pair(m_curve->data().x(0), m_curve->data().x(npts-1));
  }

  /**
   * Plot a workspace to the miniplot given a workspace pointer and
   * a specturm index.
   * 
   * @param workspace :: Pointer to the workspace
   * @param wsIndex :: The spectrum index of the workspace
   */
  void C2ETab::plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex)
  {
    using Mantid::MantidVec;

    //check if we can plot
    if( wsIndex >= workspace->getNumberHistograms() || workspace->readX(0).size() < 2 )
    {
      return;
    }

    QwtWorkspaceSpectrumData wsData(*workspace, static_cast<int>(wsIndex), false);

    if ( m_curve != NULL )
    {
      m_curve->attach(0);
      delete m_curve;
      m_curve = NULL;
    }

    size_t nhist = workspace->getNumberHistograms();
    if ( wsIndex >= nhist )
    {
      emit showMessageBox("Error: Workspace index out of range.");
    }
    else
    {
      m_curve = new QwtPlotCurve();
      m_curve->setData(wsData);
      m_curve->attach(m_plot);

      m_plot->replot();
    }
  }

    /**
   * Sets the edge bounds of plot to prevent the user inputting invalid values
   * 
   * @param min :: The lower bound property in the property browser
   * @param max :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void C2ETab::setPlotRange(QtProperty* min, QtProperty* max, const std::pair<double, double>& bounds)
  {
    m_dblManager->setMinimum(min, bounds.first);
    m_dblManager->setMaximum(min, bounds.second);
    m_dblManager->setMinimum(max, bounds.first);
    m_dblManager->setMaximum(max, bounds.second);
    m_rangeSelector->setRange(bounds.first, bounds.second);
  }

  /**
   * Set the position of the guides on the mini plot
   * 
   * @param lower :: The lower bound property in the property browser
   * @param upper :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void C2ETab::setMiniPlotGuides(QtProperty* lower, QtProperty* upper, const std::pair<double, double>& bounds)
  {
    m_dblManager->setValue(lower, bounds.first);
    m_dblManager->setValue(upper, bounds.second);
    m_rangeSelector->setMinimum(bounds.first);
    m_rangeSelector->setMaximum(bounds.second);
  }

  void C2ETab::runAlgorithm(const Mantid::API::Algorithm_sptr algorithm)
  {
    algorithm->setRethrows(true);
    m_algRunner->startAlgorithm(algorithm);
  }

  void C2ETab::algorithmFinished(bool error)
  {
    if(error)
    {
      emit showMessageBox("Error running SofQWMoments. \nSee results log for details.");
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
