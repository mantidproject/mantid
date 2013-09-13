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
      m_plot(new QwtPlot(parent)), m_curve(new QwtPlotCurve()), m_propTree(new QtTreePropertyBrowser()), 
      m_properties(), m_dblManager(new QtDoublePropertyManager()), m_intManager(new QtIntPropertyManager())
    {
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
  }
} // namespace MantidQt
