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

    QString IndirectBayesTab::tabHelpURL()
    { 
      return "http://www.mantidproject.org/IndirectBayes:" + help();
    }

    void IndirectBayesTab::runPythonScript(const QString& pyInput)
    {
      emit executePythonScript(pyInput, false);
    }

    void IndirectBayesTab::plotMiniPlot(const QString& workspace, size_t index)
    {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::MatrixWorkspace>(workspace.toStdString());
      plotMiniPlot(ws, index);
    }

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
        //showInformationBox("Error: Workspace index out of range.");
      }
     
      auto dataX = workspace->readX(wsIndex);
      auto dataY = workspace->readY(wsIndex);

      m_curve = new QwtPlotCurve();
      m_curve->setData(&dataX[0], &dataY[0], static_cast<int>(workspace->blocksize()));
      m_curve->attach(m_plot);

      m_plot->replot();
    }
  }
} // namespace MantidQt
