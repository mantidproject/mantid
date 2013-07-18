#include "MantidQtCustomInterfaces/IDATab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "boost/shared_ptr.hpp"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QString>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{

  using namespace Mantid::API;

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
   * Slot that can be called when a user edits an input.
   */
  void IDATab::inputChanged()
  {
    validate();
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
   * Run load nexus and return the workspace
   * @param filename A full path to the filename
   * @param wsname The workspace name in the ADS
   * @return A pointer to the workspace or a null pointer if the load failed
   */
  MatrixWorkspace_const_sptr IDATab::runLoadNexus(const QString & filename, const QString & wsname)
  {
    using Mantid::Kernel::Exception::NotFoundError;

    QString pyInput = "LoadNexus(r'" + filename + "', '" + wsname + "')";
    runPythonCode(pyInput);

    MatrixWorkspace_const_sptr ws;
    try
    {
      ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsname.toStdString());
    }
    catch(NotFoundError&)
    {
    }
    return ws;
  }

  /**
   * Creates and returns a "mini plot", from the given QwtPlot and QwtPlotCurve objects, as well as the given workspace
   * and workspace index.
   *
   * @param plot      :: the QwtPlot object
   * @param curve     :: the QwtPlotCurve object
   * @param workspace :: A pointer to the workspace to use
   * @param wsIndex   :: the workspace index
   *
   * @returns the resulting QwtPlotCurve object
   */
  QwtPlotCurve* IDATab::plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, const QString & workspace, size_t index)
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::MatrixWorkspace>(workspace.toStdString());
    return plotMiniplot(plot, curve, ws, index);
  }


  /**
   * Creates and returns a "mini plot", from the given QwtPlot and QwtPlotCurve objects, as well as the given workspace
   * and workspace index.
   *
   * @param plot      :: the QwtPlot object
   * @param curve     :: the QwtPlotCurve object
   * @param workspace :: A pointer to the workspace to use
   * @param wsIndex   :: the workspace index
   *
   * @returns the resulting QwtPlotCurve object
   */
  QwtPlotCurve* IDATab::plotMiniplot(QwtPlot* plot, QwtPlotCurve* curve, const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex)
  {
    if ( curve != NULL )
    {
      curve->attach(0);
      delete curve;
      curve = 0;
    }

    size_t nhist = workspace->getNumberHistograms();
    if ( wsIndex >= nhist )
    {
      showInformationBox("Error: Workspace index out of range.");
      return NULL;
    }

    using Mantid::MantidVec;
    const MantidVec & dataX = workspace->readX(wsIndex);
    const MantidVec & dataY = workspace->readY(wsIndex);

    curve = new QwtPlotCurve();
    curve->setData(&dataX[0], &dataY[0], static_cast<int>(workspace->blocksize()));
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
   * @returns a const handle to the UI form object stored in the IndirectDataAnalysis class.
   */
  const Ui::IndirectDataAnalysis & IDATab::uiForm() const
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
