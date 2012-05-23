#include "MantidQtCustomInterfaces/IDATab.h"
#include "MantidAPI\MatrixWorkspace.h"
#include "MantidAPI\AnalysisDataService.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QString>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  /**
   * Constructor.
   *
   * @param parent :: the parent widget (an IndirectDataAnalysis object).
   */
  IDATab::IDATab(QWidget * parent) : QWidget(parent), m_parent(NULL)
  {
    m_parent = dynamic_cast<IndirectDataAnalysis*>(parent);
  }

  /**
   * Sets up the tab.  
   *
   * Calls overridden version of setup() in child class.
   */
  void IDATab::setupTab() 
  {
    setup();
  }

  /**
   * Runs the tab.  
   *
   * Calls overridden version of run() in child class.
   */
  void IDATab::runTab()
  { 
    const QString error = validate();

    if( ! error.isEmpty() )
      showInformationBox(error);
    else
      run();
  }

  /**
   * Loads the tab's settings.
   *
   * Calls overridden version of loadSettings() in child class.
   *
   * @param settings :: the QSettings object from which to load
   */
  void IDATab::loadTabSettings(const QSettings & settings)
  {
    loadSettings(settings);
  }

  /**
   * Sets up the tab.
   *
   * Calls overridden version of helpURL() in child class.
   *
   * @returns a QString containing the URL of the Mantid Wiki web page for the tab.
   */
  QString IDATab::tabHelpURL()
  { 
    return "http://www.mantidproject.org/IDA:" + helpURL();
  }

  /**
   * Displays the given message in a dialog box.  Just calls the same method of parent.
   *
   * @param message :: message to display.
   */
  void IDATab::showInformationBox(const QString & message)
  {
    m_parent->showInformationBox(message);
  }

  /**
   * Runs the given Python code as a script.  Just calls the same method of parent.
   *
   * @param code      :: a QString containing the code to execute
   * @param no_output :: whether or not to show output of script
   */
  QString IDATab::runPythonCode(const QString & code, bool no_output)
  {
    return m_parent->runPythonCode(code, no_output);
  }

  /**
   * Creates and returns a "mini plot", from the given QwtPlot and QwtPlotCurve objects, as well as the given workspace
   * and workspace index.
   *
   * @param plot      :: the QwtPlot object
   * @param curve     :: the QwtPlotCurve object
   * @param workspace :: the workspace to use
   * @param wsIndex   :: the workspace index
   *
   * @returns the resulting QwtPlotCurve object
   */
  QwtPlotCurve* IDATab::plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, const std::string & workspace, size_t wsIndex)
  {
    if ( curve != NULL )
    {
      curve->attach(0);
      delete curve;
      curve = 0;
    }

    Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(workspace));

    size_t nhist = ws->getNumberHistograms();
    if ( wsIndex >= nhist )
    {
      showInformationBox("Error: Workspace index out of range.");
      return NULL;
    }

    using Mantid::MantidVec;
    const MantidVec & dataX = ws->readX(wsIndex);
    const MantidVec & dataY = ws->readY(wsIndex);

    curve = new QwtPlotCurve();
    curve->setData(&dataX[0], &dataY[0], static_cast<int>(ws->blocksize()));
    curve->attach(plot);

    plot->replot();

    return curve;
  }

  /**
   * Returns the range of the given curve data.
   *
   * @param curve :: A Qwt plot curve
   *
   * @returns A pair of doubles indicating the range
   *
   * @throws std::invalid_argument If the curve has too few points (<2) or is NULL
   */
  std::pair<double,double> IDATab::getCurveRange(QwtPlotCurve* curve)
  {
    if( !curve )
      throw std::invalid_argument("Invalid curve as argument to getCurveRange");

    size_t npts = curve->data().size();

    if( npts < 2 )
      throw std::invalid_argument("Too few points on data curve to determine range.");

    return std::make_pair(curve->data().x(0), curve->data().x(npts-1));
  }
  
  /**
   * @returns a handle to the UI form object stored in the IndirectDataAnalysis class.
   */
  Ui::IndirectDataAnalysis & IDATab::uiForm()
  {
    return m_parent->m_uiForm;
  }

  /**
   * @returns a handle to the DoubleEditorFactory object stored in the IndirectDataAnalysis class.
   */
  DoubleEditorFactory * IDATab::doubleEditorFactory() 
  { 
    return m_parent->m_dblEdFac; 
  }

  /**
   * @returns a handle to the QtCheckBoxFactory object stored in the IndirectDataAnalysis class.
   */
  QtCheckBoxFactory * IDATab::qtCheckBoxFactory() 
  { 
    return m_parent->m_blnEdFac; 
  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
