#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectBayesTab::IndirectBayesTab(QWidget * parent) : QWidget(parent),  
      m_plot(new QwtPlot(parent)), m_curve(new QwtPlotCurve()), m_rangeSelector(new MantidWidgets::RangeSelector(m_plot)),
      m_propTree(new QtTreePropertyBrowser()), m_properties(), m_dblManager(new QtDoublePropertyManager()), 
      m_dblEdFac(new DoubleEditorFactory())
    {
      m_propTree->setFactoryForManager(m_dblManager, m_dblEdFac);
      m_rangeSelector->setInfoOnly(false);

      connect(m_rangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minValueChanged(double)));
      connect(m_rangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxValueChanged(double)));
      connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateProperties(QtProperty*, double)));
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IndirectBayesTab::~IndirectBayesTab()
    {
    }

    /**
     * Method to build a URL to the appropriate page on the wiki for this tab.
     * 
     * @return The URL to the wiki page
     */
    QString IndirectBayesTab::tabHelpURL()
    { 
      return "http://www.mantidproject.org/IndirectBayes:" + help();
    }

    /**
     * Emits a signal to run a python script using the method in the parent
     * UserSubWindow
     * 
     * @param pyInput :: A string of python code to execute
     */
    void IndirectBayesTab::runPythonScript(const QString& pyInput)
    {
      emit executePythonScript(pyInput, false);
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
    void IndirectBayesTab::plotMiniPlot(const QString& workspace, size_t index)
    {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::MatrixWorkspace>(workspace.toStdString());
      plotMiniPlot(ws, index);
    }

    /**
     * Plot a workspace to the miniplot given a workspace pointer and
     * a specturm index.
     * 
     * @param workspace :: Pointer to the workspace
     * @param index :: The spectrum index of the workspace
     */
    void IndirectBayesTab::plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex)
    {
      using Mantid::MantidVec;

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
        auto dataX = workspace->readX(wsIndex);
        auto dataY = workspace->readY(wsIndex);

        m_curve = new QwtPlotCurve();
        m_curve->setData(&dataX[0], &dataY[0], static_cast<int>(workspace->blocksize()));
        m_curve->attach(m_plot);

        m_plot->replot();
      }
    }

    /**
     * Checks the workspace's intrument for a resolution parameter to use as 
     * a default for the energy range on the mini plot
     *
     * @param workspace :: Name of the workspace to use
     * @param res :: The retrieved values for the resolution parameter (if one was found)
     */
    bool IndirectBayesTab::getInstrumentResolution(const QString& workspace, std::pair<double,double>& res)
    {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::MatrixWorkspace>(workspace.toStdString());
      return getInstrumentResolution(ws, res);
    }

    /**
     * Checks the workspace's intrument for a resolution parameter to use as 
     * a default for the energy range on the mini plot
     *
     * @param workspace :: Pointer to the workspace to use
     * @param res :: The retrieved values for the resolution parameter (if one was found)
     */
    bool IndirectBayesTab::getInstrumentResolution(Mantid::API::MatrixWorkspace_const_sptr ws, std::pair<double,double>& res)
    {
      auto inst = ws->getInstrument();
      auto analyser = inst->getStringParameter("analyser");

      if(analyser.size() > 0)
      {
        auto comp = inst->getComponentByName(analyser[0]);
        auto params = comp->getNumberParameter("resolution", true);

        //set the default instrument resolution
        if(params.size() > 0)
        {
          res = std::make_pair(-params[0], params[0]);
          return true;
        }
      }

      return false;
    }

    /**
     * Gets the range of the curve plotted in the mini plot
     *
     * @param A pair containing the maximum and minimum points of the curve
     */
    std::pair<double,double> IndirectBayesTab::getCurveRange()
    {
      size_t npts = m_curve->data().size();

      if( npts < 2 )
        throw std::invalid_argument("Too few points on data curve to determine range.");

      return std::make_pair(m_curve->data().x(0), m_curve->data().x(npts-1));
    }

    /**
     * Sets the edge bounds of plot to prevent the user inputting invalid values
     * 
     * @param min :: The lower bound property in the property browser
     * @param max :: The upper bound property in the property browser
     * @param bounds :: The upper and lower bounds to be set
     */
    void IndirectBayesTab::setPlotRange(QtProperty* min, QtProperty* max, const std::pair<double, double>& bounds)
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
    void IndirectBayesTab::setMiniPlotGuides(QtProperty* lower, QtProperty* upper, const std::pair<double, double>& bounds)
    {
      m_dblManager->setValue(lower, bounds.first);
      m_dblManager->setValue(upper, bounds.second);
      m_rangeSelector->setMinimum(bounds.first);
      m_rangeSelector->setMaximum(bounds.second);
    }

    /**
     * Set the position of the lower guide on the mini plot
     * 
     * @param lower :: The lower guide property in the property browser
     * @param upper :: The upper guide property in the property browser
     * @param value :: The value of the lower guide
     */
    void IndirectBayesTab::updateLowerGuide(QtProperty* lower, QtProperty* upper, double value)
    {
      // Check if the user is setting the max less than the min
      if(value > m_dblManager->value(upper))
      {
        m_dblManager->setValue(lower, m_dblManager->value(upper));
      }
      else
      {
        m_rangeSelector->setMinimum(value);
      }
    }

    /**
     * Set the position of the upper guide on the mini plot
     * 
     * @param lower :: The lower guide property in the property browser
     * @param upper :: The upper guide property in the property browser
     * @param value :: The value of the upper guide
     */
    void IndirectBayesTab::updateUpperGuide(QtProperty* lower, QtProperty* upper, double value)
    {
      // Check if the user is setting the min greater than the max
      if(value < m_dblManager->value(lower))
      {
        m_dblManager->setValue(upper, m_dblManager->value(lower));
      }
      else
      {
        m_rangeSelector->setMaximum(value);
      }
    }
  }
} // namespace MantidQt
