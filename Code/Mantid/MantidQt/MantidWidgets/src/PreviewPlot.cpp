//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtMantidWidgets/PreviewPlot.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

#include <QAction>
#include <QBrush>
#include <QPalette>
#include <QVBoxLayout>

#include <qwt_scale_engine.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("PreviewPlot");
}


PreviewPlot::PreviewPlot(QWidget *parent, bool init) : API::MantidWidget(parent),
  m_removeObserver(*this, &PreviewPlot::handleRemoveEvent),
  m_replaceObserver(*this, &PreviewPlot::handleReplaceEvent),
  m_init(init), m_curves(),
  m_magnifyTool(NULL), m_panTool(NULL), m_zoomTool(NULL),
  m_contextMenu(new QMenu(this)), m_showLegendAction(NULL)
{
  m_uiForm.setupUi(this);
  m_uiForm.loLegend->addStretch();

  if(init)
  {
    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();
    ads.notificationCenter.addObserver(m_removeObserver);
    ads.notificationCenter.addObserver(m_replaceObserver);
  }

  // Setup plot manipulation tools
  m_zoomTool = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner, QwtPicker::AlwaysOff, m_uiForm.plot->canvas());
  m_zoomTool->setEnabled(false);

  m_panTool = new QwtPlotPanner(m_uiForm.plot->canvas());
  m_panTool->setEnabled(false);

  m_magnifyTool = new QwtPlotMagnifier(m_uiForm.plot->canvas());
  m_magnifyTool->setMouseButton(Qt::NoButton);
  m_magnifyTool->setEnabled(false);

  // Handle showing the context menu
  m_uiForm.plot->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_uiForm.plot, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

  // Create the plot tool list for context menu
  m_plotToolGroup = new QActionGroup(m_contextMenu);
  m_plotToolGroup->setExclusive(true);

  QStringList plotTools;
  plotTools << "None" << "Pan" << "Zoom";
  QList<QAction *> plotToolActions = addOptionsToMenus("Plot Tools", m_plotToolGroup, plotTools, "None");
  for(auto it = plotToolActions.begin(); it != plotToolActions.end(); ++it)
    connect(*it, SIGNAL(triggered()), this, SLOT(handleViewToolSelect()));

  // Create the reset plot view option
  QAction *resetPlotAction = new QAction("Reset Plot", m_contextMenu);
  connect(resetPlotAction, SIGNAL(triggered()), this, SLOT(resetView()));
  m_contextMenu->addAction(resetPlotAction);

  m_contextMenu->addSeparator();

  // Create the X axis type list for context menu
  m_xAxisTypeGroup = new QActionGroup(m_contextMenu);
  m_xAxisTypeGroup->setExclusive(true);

  QStringList xAxisTypes;
  xAxisTypes << "Linear" << "Logarithmic" << "Squared";
  QList<QAction *> xAxisTypeActions = addOptionsToMenus("X Axis", m_xAxisTypeGroup, xAxisTypes, "Linear");
  for(auto it = xAxisTypeActions.begin(); it != xAxisTypeActions.end(); ++it)
    connect(*it, SIGNAL(triggered()), this, SLOT(handleAxisTypeSelect()));

  // Create the X axis type list for context menu
  m_yAxisTypeGroup = new QActionGroup(m_contextMenu);
  m_yAxisTypeGroup->setExclusive(true);

  QStringList yAxisTypes;
  yAxisTypes << "Linear" << "Logarithmic";
  QList<QAction *> yAxisTypeActions = addOptionsToMenus("Y Axis", m_yAxisTypeGroup, yAxisTypes, "Linear");
  for(auto it = yAxisTypeActions.begin(); it != yAxisTypeActions.end(); ++it)
    connect(*it, SIGNAL(triggered()), this, SLOT(handleAxisTypeSelect()));

  m_contextMenu->addSeparator();

  // Create the show legend option
  m_showLegendAction = new QAction("Show Legend", m_contextMenu);
  m_showLegendAction->setCheckable(true);
  connect(m_showLegendAction, SIGNAL(toggled(bool)), this, SLOT(showLegend(bool)));
  m_contextMenu->addAction(m_showLegendAction);

  connect(this, SIGNAL(needToReplot()), this, SLOT(replot()));
  connect(this, SIGNAL(needToHardReplot()), this, SLOT(hardReplot()));
}


/**
 * Destructor
 *
 * Removes observers on the ADS.
 */
PreviewPlot::~PreviewPlot()
{
  if(m_init)
  {
    AnalysisDataService::Instance().notificationCenter.removeObserver(m_removeObserver);
    AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
  }
}


/**
 * Gets the background colour of the plot window.
 *
 * @return Plot canvas colour
 */
QColor PreviewPlot::canvasColour()
{
  return m_uiForm.plot->canvasBackground();
}


/**
 * Sets the background colour of the plot window.
 *
 * @param colour Plot canvas colour
 */
void PreviewPlot::setCanvasColour(const QColor & colour)
{
  m_uiForm.plot->setCanvasBackground(QBrush(colour));
}


/**
 * Checks to see if the plot legend is visible.
 *
 * @returns True if the legend is shown
 */
bool PreviewPlot::legendIsShown()
{
  return m_showLegendAction->isChecked();
}


/**
 * Sets the range of the given axis scale to a given range.
 *
 * @param range Pair of values for range
 * @param axisID ID of axis
 */
void PreviewPlot::setAxisRange(QPair<double, double> range, int axisID)
{
  if(range.first > range.second)
    throw std::runtime_error("Supplied range is invalid.");

  m_uiForm.plot->setAxisScale(axisID, range.first, range.second);
  replot();
}


/**
 * Gets the X range of a curve given a pointer to the workspace.
 *
 * @param ws Pointer to workspace
 */
QPair<double, double> PreviewPlot::getCurveRange(const Mantid::API::MatrixWorkspace_sptr ws)
{
  QStringList curveNames = getCurvesForWorkspace(ws);

  if(curveNames.size() == 0)
    throw std::runtime_error("Curve for workspace not found.");

  return getCurveRange(curveNames[0]);
}


/**
 * Gets the X range of a curve given its name.
 *
 * @param wsName Name of curve
 */
QPair<double, double> PreviewPlot::getCurveRange(const QString & curveName)
{
  if(!m_curves.contains(curveName))
    throw std::runtime_error("Curve not on preview plot.");

  size_t numPoints = m_curves[curveName].curve->data().size();

  if(numPoints < 2)
    return qMakePair(0.0, 0.0);

  double low = m_curves[curveName].curve->data().x(0);
  double high = m_curves[curveName].curve->data().x(numPoints - 1);

  return qMakePair(low, high);
}


/**
 * Adds a workspace to the preview plot given a pointer to it.
 *
 * @param curveName Name of curve
 * @param wsName Name of workspace in ADS
 * @param specIndex Spectrum index to plot
 * @param curveColour Colour of curve to plot
 */
void PreviewPlot::addSpectrum(const QString & curveName, const MatrixWorkspace_sptr ws,
    const size_t specIndex, const QColor & curveColour)
{
  // Remove the existing curve if it exists
  if(m_curves.contains(curveName))
    removeSpectrum(curveName);

  // Create the curve
  QwtPlotCurve * curve = addCurve(ws, specIndex, curveColour);

  // Create the curve label
  QLabel *label = new QLabel(curveName);
  label->setVisible(false);
  QPalette palette = label->palette();
  palette.setColor(label->foregroundRole(), curveColour);
  label->setPalette(palette);
  m_uiForm.loLegend->addWidget(label);
  label->setVisible(legendIsShown());

  m_curves[curveName].ws = ws;
  m_curves[curveName].curve = curve;
  m_curves[curveName].label = label;
  m_curves[curveName].colour = curveColour;
  m_curves[curveName].wsIndex = specIndex;

  // Replot
  emit needToReplot();
}


/**
 * Adds a workspace to the preview plot given its name.
 *
 * @param curveName Name of curve
 * @param wsName Name of workspace in ADS
 * @param specIndex Spectrum index to plot
 * @param curveColour Colour of curve to plot
 */
void PreviewPlot::addSpectrum(const QString & curveName, const QString & wsName,
        const size_t specIndex, const QColor & curveColour)
{
  // Try to get a pointer from the name
  std::string wsNameStr = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());

  if(!ws)
    throw std::runtime_error(wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  addSpectrum(curveName, ws, specIndex, curveColour);
}


/**
 * Removes spectra from a gievn workspace from the plot given a pointer to it.
 *
 * If multiple curves are plotted form the smae workspace then all wil lbe removed.
 *
 * @param ws Pointer to workspace
 */
void PreviewPlot::removeSpectrum(const MatrixWorkspace_sptr ws)
{
  QStringList curveNames = getCurvesForWorkspace(ws);

  for(auto it = curveNames.begin(); it != curveNames.end(); ++it)
    removeSpectrum(*it);
}


/**
 * Removes spectra from a gievn workspace from the plot given its name.
 *
 * @param wsName Name of curve
 */
void PreviewPlot::removeSpectrum(const QString & curveName)
{
  // Remove the curve object and legend label
  if(m_curves.contains(curveName))
  {
    removeCurve(m_curves[curveName].curve);
    m_uiForm.loLegend->removeWidget(m_curves[curveName].label);
    delete m_curves[curveName].label;
  }

  // Get the curve from the map
  auto it = m_curves.find(curveName);

  // Remove the curve from the map
  if(it != m_curves.end())
    m_curves.erase(it);
}


/**
 * Shows or hides the plot legend.
 *
 * @param show If the legend should be shown
 */
void PreviewPlot::showLegend(bool show)
{
  m_showLegendAction->setChecked(show);

  for(auto it = m_curves.begin(); it != m_curves.end(); ++it)
    it.value().label->setVisible(show);
}


/**
 * Toggles the pan plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void PreviewPlot::togglePanTool(bool enabled)
{
  // First disbale the zoom tool
  if(enabled && m_zoomTool->isEnabled())
    m_zoomTool->setEnabled(false);

  m_panTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}


/**
 * Toggles the zoom plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void PreviewPlot::toggleZoomTool(bool enabled)
{
  // First disbale the pan tool
  if(enabled && m_panTool->isEnabled())
    m_panTool->setEnabled(false);

  m_zoomTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}


/**
 * Resets the view to a sensible default.
 */
void PreviewPlot::resetView()
{
  // Auto scale the axis
  m_uiForm.plot->setAxisAutoScale(QwtPlot::xBottom);
  m_uiForm.plot->setAxisAutoScale(QwtPlot::yLeft);

  // Set this as the default zoom level
  m_zoomTool->setZoomBase(true);
}


/**
 * Resizes the X axis scale range to exactly fir the curves currently
 * plotted on it.
 */
void PreviewPlot::resizeX()
{
  double low = DBL_MAX;
  double high = DBL_MIN;

  for(auto it = m_curves.begin(); it != m_curves.end(); ++it)
  {
    auto range = getCurveRange(it.key());

    if(range.first < low)
      low = range.first;

    if(range.second > high)
      high = range.second;
  }

  setAxisRange(qMakePair(low, high), QwtPlot::xBottom);
}


/**
 * Removes all curves from the plot.
 */
void PreviewPlot::clear()
{
  for(auto it = m_curves.begin(); it != m_curves.end(); ++it)
  {
    removeCurve(it.value().curve);
    m_uiForm.loLegend->removeWidget(it.value().label);
    delete it.value().label;
  }

  m_curves.clear();

  emit needToReplot();
}


/**
 * Replots the curves shown on the plot.
 */
void PreviewPlot::replot()
{
  m_uiForm.plot->replot();
}


/**
 * Removes all curves and re-adds them.
 */
void PreviewPlot::hardReplot()
{
  QStringList keys = m_curves.keys();

  for(auto it = keys.begin(); it != keys.end(); ++it)
  {
    removeCurve(m_curves[*it].curve);
    m_curves[*it].curve = addCurve(m_curves[*it].ws, m_curves[*it].wsIndex, m_curves[*it].colour);
  }

  replot();
}


/**
 * Handle a workspace being deleted from ADS.
 *
 * Removes it from the plot (via removeSpectrum).
 *
 * @param pNF Poco notification
 */
void PreviewPlot::handleRemoveEvent(WorkspacePreDeleteNotification_ptr pNf)
{
  MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(pNf->object());

  // Ignore non matrix worksapces
  if(!ws)
    return;

  // Remove the workspace
  removeSpectrum(ws);

  emit needToReplot();
}


/**
 * Handle a workspace being modified in ADS.
 *
 * Removes the existing curve and re adds it to reflect new data.
 *
 * @param pNf Poco notification
 */
void PreviewPlot::handleReplaceEvent(WorkspaceAfterReplaceNotification_ptr pNf)
{
  MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(pNf->object());

  // Ignore non matrix worksapces
  if(!ws)
    return;

  if(getCurvesForWorkspace(ws).size() > 0)
    emit needToHardReplot();
}


/**
 * Creates a new curve and adds it to the plot.
 *
 * @param ws Worksapce pointer
 * @param specIndex Index of histogram to plot
 * @param curveColour Colour of curve
 * @return Pointer to new curve
 */
QwtPlotCurve * PreviewPlot::addCurve(MatrixWorkspace_sptr ws, const size_t specIndex,
   const QColor & curveColour)
{
  // Check the spectrum index is in range
  if(specIndex >= ws->getNumberHistograms())
    throw std::runtime_error("Workspace index is out of range, cannot plot.");

  // Check the X axis is large enough
  if(ws->readX(0).size() < 2)
    throw std::runtime_error("X axis is too small to generate a histogram plot.");

  // Convert X axis to squared if needed
  if(getAxisType(QwtPlot::xBottom) == "Squared")
  {
    Mantid::API::IAlgorithm_sptr convertXAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertAxisByFormula");
    convertXAlg->initialize();
    convertXAlg->setChild(true);
    convertXAlg->setLogging(false);
    convertXAlg->setProperty("InputWorkspace", ws);
    convertXAlg->setProperty("OutputWorkspace", "__PreviewPlot_Anon");
    convertXAlg->setProperty("Axis", "X");
    convertXAlg->setProperty("Formula", "x^2");
    convertXAlg->execute();
    ws = convertXAlg->getProperty("OutputWorkspace");
  }

  // Create the plot data
  QwtWorkspaceSpectrumData wsData(*ws, static_cast<int>(specIndex), false, false);

  // Create the new curve
  QwtPlotCurve *curve = new QwtPlotCurve();
  curve->setData(wsData);
  curve->setPen(curveColour);
  curve->attach(m_uiForm.plot);

  return curve;
}


/**
 * Removes a curve from the plot.
 *
 * @param curve Curve to remove
 */
void PreviewPlot::removeCurve(QwtPlotCurve * curve)
{
  if(!curve)
    return;

  // Take it off the plot
  curve->attach(NULL);

  // Delete it
  delete curve;
  curve = NULL;
}


/**
 * Helper function for adding a set of items to an exclusive menu oon the context menu.
 *
 * @param menuName Name of sub menu
 * @param group Pointer to ActionGroup
 * @param items List of item names
 * @param defaultItem Default item name
 * @return List of Actions added
 */
QList<QAction *> PreviewPlot::addOptionsToMenus(QString menuName, QActionGroup *group, QStringList items, QString defaultItem)
{
  QMenu *menu = new QMenu(m_contextMenu);

  for(auto it = items.begin(); it != items.end(); ++it)
  {
    QAction *action = new QAction(*it, menu);
    action->setCheckable(true);

    // Add to the menu and action group
    group->addAction(action);
    menu->addAction(action);

    // Select default
    action->setChecked(*it == defaultItem);
  }

  QAction *menuAction = new QAction(menuName, menu);
  menuAction->setMenu(menu);
  m_contextMenu->addAction(menuAction);

  return group->actions();
}


/**
 * Returns the type of axis scale specified for a giev axis.
 *
 * @param axisID ID of axis
 * @return Axis type as string
 */
QString PreviewPlot::getAxisType(int axisID)
{
  QString axisType("Linear");
  QAction * selectedAxisType = NULL;

  if(axisID == QwtPlot::xBottom)
    selectedAxisType = m_xAxisTypeGroup->checkedAction();
  else if (axisID == QwtPlot::yLeft)
    selectedAxisType = m_yAxisTypeGroup->checkedAction();
  else
    return QString();

  if(selectedAxisType)
    axisType = selectedAxisType->text();

  return axisType;
}


/**
 * Gets a list of curve names that are plotted form the given workspace.
 *
 * @param Pointer to workspace
 * @return List of curve names
 */
QStringList PreviewPlot::getCurvesForWorkspace(const MatrixWorkspace_sptr ws)
{
  QStringList curveNames;

  for(auto it = m_curves.begin(); it != m_curves.end(); ++it)
  {
    if(it.value().ws == ws)
      curveNames << it.key();
  }

  return curveNames;
}


/**
 * Handles displaying the context menu when a user right clicks on the plot.
 *
 * @param position Position at which to show menu
 */
void PreviewPlot::showContextMenu(QPoint position)
{
  // Show the context menu
  m_contextMenu->popup(m_uiForm.plot->mapToGlobal(position));
}


/**
 * Handles the view tool being selected from the context menu.
 */
void PreviewPlot::handleViewToolSelect()
{
  QAction *selectedPlotType = m_plotToolGroup->checkedAction();
  if(!selectedPlotType)
    return;

  QString selectedTool = selectedPlotType->text();
  if(selectedTool == "None")
  {
    togglePanTool(false);
    toggleZoomTool(false);
  }
  else if(selectedTool == "Pan")
  {
    togglePanTool(true);
  }
  else if(selectedTool == "Zoom")
  {
    toggleZoomTool(true);
  }
}


/**
 * Handles a change in the plot axis type.
 */
void PreviewPlot::handleAxisTypeSelect()
{
  // Determine the type of engine to use for each axis
  QString xAxisType = getAxisType(QwtPlot::xBottom);
  QString yAxisType = getAxisType(QwtPlot::yLeft);

  QwtScaleEngine *xEngine = NULL;
  QwtScaleEngine *yEngine = NULL;

  // Get the X axis engine
  if(xAxisType == "Linear")
  {
    xEngine = new QwtLinearScaleEngine();
  }
  else if(xAxisType == "Logarithmic")
  {
    xEngine = new QwtLog10ScaleEngine();
  }
  else if(xAxisType == "Squared")
  {
    xEngine = new QwtLinearScaleEngine();
  }

  // Get the Y axis engine
  if(yAxisType == "Linear")
  {
    yEngine = new QwtLinearScaleEngine();
  }
  else if(yAxisType == "Logarithmic")
  {
    yEngine = new QwtLog10ScaleEngine();
  }

  // Set the axis scale engines
  if(xEngine)
    m_uiForm.plot->setAxisScaleEngine(QwtPlot::xBottom, xEngine);

  if(yEngine)
    m_uiForm.plot->setAxisScaleEngine(QwtPlot::yLeft, yEngine);

  // Update the plot
  emit needToHardReplot();
}
