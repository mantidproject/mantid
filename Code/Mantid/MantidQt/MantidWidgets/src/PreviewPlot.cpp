//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtMantidWidgets/PreviewPlot.h"

#include "MantidAPI/AnalysisDataService.h"
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
  m_init(init), m_plot(new QwtPlot(this)), m_curves(),
  m_magnifyTool(NULL), m_panTool(NULL), m_zoomTool(NULL),
  m_contextMenu(new QMenu(this)), m_showLegendAction(NULL), m_legendLayout(NULL)
{
  if(init)
  {
    AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();
    ads.notificationCenter.addObserver(m_removeObserver);
    ads.notificationCenter.addObserver(m_replaceObserver);

    QBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_legendLayout = new QHBoxLayout(mainLayout);
    m_legendLayout->addStretch();
    mainLayout->addItem(m_legendLayout);

    mainLayout->addWidget(m_plot);

    this->setLayout(mainLayout);
  }

  // Setup plot manipulation tools
  m_zoomTool = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner, QwtPicker::AlwaysOff, m_plot->canvas());
  m_zoomTool->setEnabled(false);

  m_panTool = new QwtPlotPanner(m_plot->canvas());
  m_panTool->setEnabled(false);

  m_magnifyTool = new QwtPlotMagnifier(m_plot->canvas());
  m_magnifyTool->setMouseButton(Qt::NoButton);
  m_magnifyTool->setEnabled(false);

  // Handle showing the context menu
  m_plot->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_plot, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

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
  if(m_plot)
    return m_plot->canvasBackground();

  return QColor();
}


/**
 * Sets the background colour of the plot window.
 *
 * @param colour Plot canvas colour
 */
void PreviewPlot::setCanvasColour(const QColor & colour)
{
  if(m_plot)
    m_plot->setCanvasBackground(QBrush(colour));
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

  m_plot->setAxisScale(axisID, range.first, range.second);
  replot();
}


/**
 * Gets the X range of a curve given a pointer to the workspace.
 *
 * @param ws Pointer to workspace
 */
QPair<double, double> PreviewPlot::getCurveRange(const Mantid::API::MatrixWorkspace_const_sptr ws)
{
  if(!m_curves.contains(ws))
    throw std::runtime_error("Workspace not on preview plot.");

  size_t numPoints = m_curves[ws].first->data().size();

  if(numPoints < 2)
    return qMakePair(0.0, 0.0);

  double low = m_curves[ws].first->data().x(0);
  double high = m_curves[ws].first->data().x(numPoints - 1);

  return qMakePair(low, high);
}


/**
 * Gets the X range of a curve given its name.
 *
 * @param wsName Name of workspace
 */
QPair<double, double> PreviewPlot::getCurveRange(const QString & wsName)
{
  // Try to get a pointer from the name
  std::string wsNameStr = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsName.toStdString());

  if(!ws)
    throw std::runtime_error(wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  return getCurveRange(ws);
}


/**
 * Adds a workspace to the preview plot given a pointer to it.
 *
 * @param curveName Name of curve
 * @param wsName Name of workspace in ADS
 * @param specIndex Spectrum index to plot
 * @param curveColour Colour of curve to plot
 */
void PreviewPlot::addSpectrum(const QString & curveName, const MatrixWorkspace_const_sptr ws,
    const size_t specIndex, const QColor & curveColour)
{
  // Check the spectrum index is in range
  if(specIndex >= ws->getNumberHistograms())
    throw std::runtime_error("Workspace index is out of range, cannot plot.");

  // Check the X axis is large enough
  if(ws->readX(0).size() < 2)
    throw std::runtime_error("X axis is too small to generate a histogram plot.");

  // Create the plot data
  const bool logScale(false), distribution(false);
  QwtWorkspaceSpectrumData wsData(*ws, static_cast<int>(specIndex), logScale, distribution);

  // Remove any existing curves
  if(m_curves.contains(ws))
    removeCurve(m_curves[ws].first);

  // Create the new curve
  QwtPlotCurve *curve = new QwtPlotCurve();
  curve->setData(wsData);
  curve->setPen(curveColour);
  curve->attach(m_plot);

  // Create the curve label
  QLabel *label = new QLabel(curveName);
  QPalette palette = label->palette();
  palette.setColor(label->foregroundRole(), curveColour);
  label->setPalette(palette);
  label->setVisible(legendIsShown());
  m_legendLayout->addWidget(label);

  m_curves[ws] = qMakePair(curve, label);

  // Replot
  m_plot->replot();
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
  auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsName.toStdString());

  if(!ws)
    throw std::runtime_error(wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  addSpectrum(curveName, ws, specIndex, curveColour);
}


/**
 * Removes spectra from a gievn workspace from the plot given a pointer to it.
 *
 * @param ws Pointer to workspace
 */
void PreviewPlot::removeSpectrum(const MatrixWorkspace_const_sptr ws)
{
  // Remove the curve object and legend label
  if(m_curves.contains(ws))
  {
    removeCurve(m_curves[ws].first);
    m_legendLayout->removeWidget(m_curves[ws].second);
    delete m_curves[ws].second;
  }

  // Get the curve from the map
  auto it = m_curves.find(ws);

  // Remove the curve from the map
  if(it != m_curves.end())
    m_curves.erase(it);
}


/**
 * Removes spectra from a gievn workspace from the plot given its name.
 *
 * @param wsName Name of workspace
 */
void PreviewPlot::removeSpectrum(const QString & wsName)
{
  // Try to get a pointer from the name
  std::string wsNameStr = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsNameStr);

  if(!ws)
    throw std::runtime_error(wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  removeSpectrum(ws);
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
    it.value().second->setVisible(show);
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
  m_plot->setAxisAutoScale(QwtPlot::xBottom);
  m_plot->setAxisAutoScale(QwtPlot::yLeft);

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
    removeCurve(it.value().first);
    m_legendLayout->removeWidget(it.value().second);
    delete it.value().second;
  }

  m_curves.clear();

  replot();
}


/**
 * Replots the curves shown on the plot.
 */
void PreviewPlot::replot()
{
  m_plot->replot();
}


void PreviewPlot::handleRemoveEvent(WorkspacePreDeleteNotification_ptr pNf)
{
  //TODO
}


void PreviewPlot::handleReplaceEvent(WorkspaceAfterReplaceNotification_ptr pNf)
{
  //TODO
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

  QAction *menuAction = new QAction(menuName, this);
  menuAction->setMenu(menu);
  m_contextMenu->addAction(menuAction);

  return group->actions();
}


/**
 * Handles displaying the context menu when a user right clicks on the plot.
 *
 * @param position Position at which to show menu
 */
void PreviewPlot::showContextMenu(QPoint position)
{
  // Show the context menu
  m_contextMenu->popup(m_plot->mapToGlobal(position));
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
  QString xAxisType("Linear");
  QString yAxisType("Linear");

  QAction *selectedXAxisType = m_xAxisTypeGroup->checkedAction();
  if(selectedXAxisType)
    xAxisType = selectedXAxisType->text();

  QAction *selectedYAxisType = m_yAxisTypeGroup->checkedAction();
  if(selectedYAxisType)
    yAxisType = selectedYAxisType->text();

  QwtScaleEngine *xEngine = NULL;
  QwtScaleEngine *yEngine = NULL;

  if(xAxisType == "Linear")
    xEngine = new QwtLinearScaleEngine();
  else if(xAxisType == "Logarithmic")
    xEngine = new QwtLog10ScaleEngine();

  if(yAxisType == "Linear")
    yEngine = new QwtLinearScaleEngine();
  else if(yAxisType == "Logarithmic")
    yEngine = new QwtLog10ScaleEngine();

  if(xEngine)
    m_plot->setAxisScaleEngine(QwtPlot::xBottom, xEngine);

  if(yEngine)
    m_plot->setAxisScaleEngine(QwtPlot::yLeft, yEngine);

  m_plot->replot();
}
