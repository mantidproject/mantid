// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtWidgets/Plotting/Qwt/PreviewPlot.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>

#include <QAction>
#include <QPalette>

#include <qwt_array.h>
#include <qwt_data.h>
#include <qwt_scale_engine.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("PreviewPlot");
bool isNegative(double v) { return v <= 0.0; }
} // namespace

PreviewPlot::PreviewPlot(QWidget *parent, bool init)
    : API::MantidWidget(parent),
      m_removeObserver(*this, &PreviewPlot::handleRemoveEvent),
      m_replaceObserver(*this, &PreviewPlot::handleReplaceEvent), m_init(init),
      m_curves(), m_magnifyTool(nullptr), m_panTool(nullptr),
      m_zoomTool(nullptr), m_contextMenu(new QMenu(this)),
      m_showLegendAction(nullptr), m_showErrorsMenuAction(nullptr),
      m_showErrorsMenu(nullptr), m_errorBarOptionCache(), m_curveStyle(),
      m_curveSymbol() {
  m_uiForm.setupUi(this);
  m_uiForm.loLegend->addStretch();
  watchADS(init);

  // Setup plot manipulation tools
  m_zoomTool =
      new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
                        QwtPicker::DragSelection | QwtPicker::CornerToCorner,
                        QwtPicker::AlwaysOff, m_uiForm.plot->canvas());
  m_zoomTool->setEnabled(false);

  m_panTool = new QwtPlotPanner(m_uiForm.plot->canvas());
  m_panTool->setEnabled(false);

  m_magnifyTool = new QwtPlotMagnifier(m_uiForm.plot->canvas());
  m_magnifyTool->setMouseButton(Qt::NoButton);
  m_magnifyTool->setEnabled(false);

  // Handle showing the context menu
  m_uiForm.plot->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_uiForm.plot, SIGNAL(customContextMenuRequested(QPoint)), this,
          SLOT(showContextMenu(QPoint)));

  // Create the plot tool list for context menu
  m_plotToolGroup = new QActionGroup(m_contextMenu);
  m_plotToolGroup->setExclusive(true);

  QStringList plotTools;
  plotTools << "None"
            << "Pan"
            << "Zoom";
  QList<QAction *> plotToolActions =
      addOptionsToMenus("Plot Tools", m_plotToolGroup, plotTools, "None");
  for (auto &plotToolAction : plotToolActions)
    connect(plotToolAction, SIGNAL(triggered()), this,
            SLOT(handleViewToolSelect()));

  // Create the reset plot view option
  QAction *resetPlotAction = new QAction("Reset Plot", m_contextMenu);
  connect(resetPlotAction, SIGNAL(triggered()), this, SLOT(resetView()));
  m_contextMenu->addAction(resetPlotAction);

  m_contextMenu->addSeparator();

  // Create the X axis type list for context menu
  m_xAxisTypeGroup = new QActionGroup(m_contextMenu);
  m_xAxisTypeGroup->setExclusive(true);

  QStringList xAxisTypes;
  xAxisTypes << "Linear"
             << "Logarithmic"
             << "Squared";
  QList<QAction *> xAxisTypeActions =
      addOptionsToMenus("X Axis", m_xAxisTypeGroup, xAxisTypes, "Linear");
  for (auto &xAxisTypeAction : xAxisTypeActions)
    connect(xAxisTypeAction, SIGNAL(triggered()), this,
            SLOT(handleAxisTypeSelect()));

  // Create the X axis type list for context menu
  m_yAxisTypeGroup = new QActionGroup(m_contextMenu);
  m_yAxisTypeGroup->setExclusive(true);

  QStringList yAxisTypes;
  yAxisTypes << "Linear"
             << "Logarithmic";
  QList<QAction *> yAxisTypeActions =
      addOptionsToMenus("Y Axis", m_yAxisTypeGroup, yAxisTypes, "Linear");
  for (auto &yAxisTypeAction : yAxisTypeActions)
    connect(yAxisTypeAction, SIGNAL(triggered()), this,
            SLOT(handleAxisTypeSelect()));

  m_contextMenu->addSeparator();

  // Create the show legend option
  m_showLegendAction = new QAction("Show Legend", m_contextMenu);
  m_showLegendAction->setCheckable(true);
  connect(m_showLegendAction, SIGNAL(toggled(bool)), this,
          SLOT(showLegend(bool)));
  m_contextMenu->addAction(m_showLegendAction);

  // Create the show errors option
  m_showErrorsMenuAction = new QAction("Error Bars:", m_contextMenu);
  m_showErrorsMenu = new QMenu(m_contextMenu);
  m_showErrorsMenuAction->setMenu(m_showErrorsMenu);
  m_contextMenu->addAction(m_showErrorsMenuAction);

  connect(this, SIGNAL(needToReplot()), this, SLOT(replot()));
  connect(this, SIGNAL(needToHardReplot()), this, SLOT(hardReplot()));
  connect(this, SIGNAL(workspaceRemoved(Mantid::API::MatrixWorkspace_sptr)),
          this, SLOT(removeWorkspace(Mantid::API::MatrixWorkspace_sptr)),
          Qt::QueuedConnection);
}

/**
 * Destructor
 *
 * Removes observers on the ADS.
 */
PreviewPlot::~PreviewPlot() { watchADS(!m_init); }

/**
 * Enable/disable the ADS observers
 * @param on If true ADS observers are enabled else they are disabled
 */
void PreviewPlot::watchADS(bool on) {
  auto &notificationCenter = AnalysisDataService::Instance().notificationCenter;
  if (on) {
    notificationCenter.addObserver(m_removeObserver);
    notificationCenter.addObserver(m_replaceObserver);
  } else {
    notificationCenter.removeObserver(m_replaceObserver);
    notificationCenter.removeObserver(m_removeObserver);
  }
}

/**
 * Gets the canvas.
 *
 * @return The preview plot canvas
 */
QwtPlotCanvas *PreviewPlot::canvas() const { return m_uiForm.plot->canvas(); }

/**
 * Gets the QwtPlot.
 *
 * @return The QwtPlot.
 */
QwtPlot *PreviewPlot::getPlot() const { return m_uiForm.plot; }

/**
 * Gets the background colour of the plot window.
 *
 * @return Plot canvas colour
 */
QColor PreviewPlot::canvasColour() { return m_uiForm.plot->canvasBackground(); }

/**
 * Sets the background colour of the plot window.
 *
 * @param colour Plot canvas colour
 */
void PreviewPlot::setCanvasColour(const QColor &colour) {
  m_uiForm.plot->setCanvasBackground(colour);
}

/**
 * Checks to see if the plot legend is visible.
 *
 * @returns True if the legend is shown
 */
bool PreviewPlot::legendIsShown() { return m_showLegendAction->isChecked(); }

/**
 * Gets a list of curves that have their error bars shown.
 *
 * @return List of curve names with error bars shown
 */
QStringList PreviewPlot::getShownErrorBars() {
  QStringList curvesWithErrors;

  for (auto it = m_curves.begin(); it != m_curves.end(); ++it) {
    if (it.value().showErrorsAction->isChecked())
      curvesWithErrors << it.key();
  }

  return curvesWithErrors;
}

/**
 * Sets the range of the given axis scale to a given range.
 *
 * @param range Pair of values for range
 * @param axisID ID of axis
 */
void PreviewPlot::setAxisRange(QPair<double, double> range, AxisID axisID) {
  if (range.first > range.second)
    throw std::runtime_error("Supplied range is invalid.");

  m_uiForm.plot->setAxisScale(toQwtAxis(axisID), range.first, range.second);
  emit needToReplot();
}

/**
 * Gets the X range of a curve given a pointer to the workspace.
 *
 * @param ws Pointer to workspace
 */
QPair<double, double>
PreviewPlot::getCurveRange(const Mantid::API::MatrixWorkspace_sptr ws) {
  QStringList curveNames = getCurvesForWorkspace(ws);

  if (curveNames.size() == 0)
    throw std::runtime_error("Curve for workspace not found.");

  return getCurveRange(curveNames[0]);
}

/**
 * Gets the X range of a curve given its name.
 *
 * @param curveName Name of curve
 */
QPair<double, double> PreviewPlot::getCurveRange(const QString &curveName) {
  if (!m_curves.contains(curveName))
    throw std::runtime_error("Curve not on preview plot.");

  size_t numPoints = m_curves[curveName].curve->data().size();

  if (numPoints < 2)
    return qMakePair(0.0, 0.0);

  double low = m_curves[curveName].curve->data().x(0);
  double high = m_curves[curveName].curve->data().x(numPoints - 1);

  return qMakePair(low, high);
}

/**
 * Adds a workspace to the preview plot given a pointer to it.
 *
 * @param curveName Name of curve
 * @param ws Pointer to the workspace
 * @param wsIndex workspace index to plot
 * @param curveColour Colour of curve to plot
 */
void PreviewPlot::addSpectrum(const QString &curveName,
                              const MatrixWorkspace_sptr &ws,
                              const size_t wsIndex, const QColor &curveColour,
                              const QHash<QString, QVariant> &plotKwargs) {
  UNUSED_ARG(plotKwargs);

  if (curveName.isEmpty()) {
    g_log.warning("Cannot plot with empty curve name");
    return;
  }

  if (!ws) {
    g_log.warning("Cannot plot null workspace");
    return;
  }

  // Remove the existing curve if it exists
  if (m_curves.contains(curveName))
    removeSpectrum(curveName);

  // Add the error bar option
  m_curves[curveName].showErrorsAction =
      new QAction(curveName, m_showErrorsMenuAction);
  m_curves[curveName].showErrorsAction->setCheckable(true);
  if (m_errorBarOptionCache.contains(curveName))
    m_curves[curveName].showErrorsAction->setChecked(
        m_errorBarOptionCache[curveName]);
  connect(m_curves[curveName].showErrorsAction, SIGNAL(toggled(bool)), this,
          SIGNAL(needToHardReplot()));
  m_showErrorsMenu->addAction(m_curves[curveName].showErrorsAction);

  // Create the curve
  addCurve(m_curves[curveName], ws, wsIndex, curveColour, curveName);

  // Create the curve label
  QLabel *label = new QLabel(curveName);
  label->setVisible(false);
  QPalette palette = label->palette();
  palette.setColor(label->foregroundRole(), curveColour);
  label->setPalette(palette);
  m_uiForm.loLegend->addWidget(label);
  label->setVisible(legendIsShown());

  m_curves[curveName].ws = ws;
  m_curves[curveName].label = label;
  m_curves[curveName].colour = curveColour;
  m_curves[curveName].wsIndex = wsIndex;

  // Replot
  emit needToReplot();
}

/**
 * Adds a workspace to the preview plot given its name.
 *
 * @param curveName Name of curve
 * @param wsName Name of workspace in ADS
 * @param wsIndex workspace index to plot
 * @param curveColour Colour of curve to plot
 */
void PreviewPlot::addSpectrum(const QString &curveName, const QString &wsName,
                              const size_t wsIndex, const QColor &curveColour,
                              const QHash<QString, QVariant> &plotKwargs) {
  if (wsName.isEmpty()) {
    g_log.error("Cannot plot with empty workspace name");
    return;
  }

  // Try to get a pointer from the name
  std::string wsNameStr = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());

  if (!ws)
    throw std::runtime_error(
        wsNameStr + " is not a MatrixWorkspace, not supported by PreviewPlot.");

  addSpectrum(curveName, ws, wsIndex, curveColour, plotKwargs);
}

/**
 * Removes spectra from a given workspace from the plot given a pointer to it.
 *
 * If multiple curves are plotted form the same workspace then all will be
 * removed.
 *
 * @param ws Pointer to workspace
 */
void PreviewPlot::removeSpectrum(const MatrixWorkspace_sptr ws) {
  if (!ws) {
    g_log.error("Cannot remove curve for null workspace");
    return;
  }

  QStringList curveNames = getCurvesForWorkspace(ws);

  for (auto &curveName : curveNames)
    removeSpectrum(curveName);
}

/**
 * Removes spectra from a given workspace from the plot given its name.
 *
 * @param curveName Name of curve
 */
void PreviewPlot::removeSpectrum(const QString &curveName) {
  if (curveName.isEmpty()) {
    g_log.error("Cannot remove curve with empty name");
    return;
  }

  // Remove the curve object and legend label
  if (m_curves.contains(curveName)) {
    removeCurve(m_curves[curveName].curve);
    removeCurve(m_curves[curveName].errorCurve);
    m_uiForm.loLegend->removeWidget(m_curves[curveName].label);
    m_errorBarOptionCache[curveName] =
        m_curves[curveName].showErrorsAction->isChecked();
    m_showErrorsMenu->removeAction(m_curves[curveName].showErrorsAction);
    delete m_curves[curveName].showErrorsAction;
    delete m_curves[curveName].label;
  }

  // Get the curve from the map
  auto it = m_curves.find(curveName);

  // Remove the curve from the map
  if (it != m_curves.end())
    m_curves.erase(it);

  emit needToReplot();
}

/**
 * Checks to see if a given curve name is present on the plot.
 *
 * @param curveName Curve name
 * @return True if curve is on plot
 */
bool PreviewPlot::hasCurve(const QString &curveName) {
  return m_curves.contains(curveName);
}

/**
 * Creates a RangeSelector, adds it to the plot and stores it.
 *
 * @param rsName Name of range selector
 * @param type Type of range selector to add
 * @return RangeSelector object
 */
RangeSelector *PreviewPlot::addRangeSelector(const QString &rsName,
                                             RangeSelector::SelectType type) {
  if (hasRangeSelector(rsName))
    throw std::runtime_error("RangeSelector already exists on PreviewPlot");

  m_rangeSelectors[rsName] =
      new MantidWidgets::RangeSelector(m_uiForm.plot, type);
  m_rsVisibility[rsName] = m_rangeSelectors[rsName]->isVisible();

  return m_rangeSelectors[rsName];
}

/**
 * Gets a RangeSelector.
 *
 * @param rsName Name of range selector
 * @return RangeSelector object
 */
RangeSelector *PreviewPlot::getRangeSelector(const QString &rsName) {
  if (!hasRangeSelector(rsName))
    throw std::runtime_error("RangeSelector not found on PreviewPlot");

  return m_rangeSelectors[rsName];
}

/**
 * Removes a RangeSelector from the plot.
 *
 * @param rsName Name of range selector
 * @param del If the object should be deleted
 */
void PreviewPlot::removeRangeSelector(const QString &rsName, bool del = true) {
  if (!hasRangeSelector(rsName))
    return;

  if (del)
    delete m_rangeSelectors[rsName];

  m_rangeSelectors.remove(rsName);
}

/**
 * Checks to see if a range selector with a given name is on the plot.
 *
 * @param rsName Name of range selector
 * @return True if the plot has a range selector with the given name
 */
bool PreviewPlot::hasRangeSelector(const QString &rsName) {
  return m_rangeSelectors.contains(rsName);
}

/**
 * Shows or hides the plot legend.
 *
 * @param show If the legend should be shown
 */
void PreviewPlot::showLegend(bool show) {
  m_showLegendAction->setChecked(show);

  for (auto it = m_curves.begin(); it != m_curves.end(); ++it)
    it.value().label->setVisible(show);
}

/**
 * Show error bars for the given curves and set a marker that if any curves
 * added with a given name will have error bars.
 *
 * @param curveNames List of curve names to show error bars of
 */
void PreviewPlot::setLinesWithErrors(const QStringList &curveNames) {
  for (const auto &curveName : curveNames) {
    m_errorBarOptionCache[curveName] = true;

    if (m_curves.contains(curveName))
      m_curves[curveName].showErrorsAction->setChecked(true);
  }

  emit needToHardReplot();
}

/**
 * Toggles the pan plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void PreviewPlot::togglePanTool(bool enabled) {
  // First disbale the zoom tool
  if (enabled && m_zoomTool->isEnabled())
    m_zoomTool->setEnabled(false);

  m_panTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}

/**
 * Toggles the zoom plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void PreviewPlot::toggleZoomTool(bool enabled) {
  // First disbale the pan tool
  if (enabled && m_panTool->isEnabled())
    m_panTool->setEnabled(false);

  m_zoomTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}

/**
 * Resets the view to a sensible default.
 */
void PreviewPlot::resetView() {
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
void PreviewPlot::resizeX() {
  double low = DBL_MAX;
  double high = DBL_MIN;

  for (auto it = m_curves.begin(); it != m_curves.end(); ++it) {
    auto range = getCurveRange(it.key());

    if (range.first < low)
      low = range.first;

    if (range.second > high)
      high = range.second;
  }

  setAxisRange(qMakePair(low, high), AxisID::XBottom);
}

/**
 * Removes all curves from the plot.
 */
void PreviewPlot::clear() {
  for (auto it = m_curves.begin(); it != m_curves.end(); ++it) {
    removeCurve(it.value().curve);
    removeCurve(it.value().errorCurve);
    m_uiForm.loLegend->removeWidget(it.value().label);
    m_errorBarOptionCache[it.key()] = it.value().showErrorsAction->isChecked();
    m_showErrorsMenu->removeAction(it.value().showErrorsAction);
    delete it.value().showErrorsAction;
    delete it.value().label;
  }

  m_curves.clear();

  emit needToReplot();
}

/**
 * Replots the curves shown on the plot.
 */
void PreviewPlot::replot() { m_uiForm.plot->replot(); }

/**
 * Removes all curves and re-adds them.
 */
void PreviewPlot::hardReplot() {
  QStringList keys = m_curves.keys();

  for (auto &key : keys) {
    removeCurve(m_curves[key].curve);
    removeCurve(m_curves[key].errorCurve);
    addCurve(m_curves[key], m_curves[key].ws, m_curves[key].wsIndex,
             m_curves[key].colour);
  }

  emit needToReplot();
}

/**
 * Handle a notifcation about a workspace being deleted from ADS.
 * @param pNf Poco notification
 */
void PreviewPlot::handleRemoveEvent(WorkspacePreDeleteNotification_ptr pNf) {
  auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(pNf->object());

  if (ws)
    // emit a queued signal to the queued slot
    emit workspaceRemoved(ws);
}

/**
 * Remove the spectrum & replot when a workspace is removed.
 *
 * @param ws the workspace that is being removed.
 */
void PreviewPlot::removeWorkspace(MatrixWorkspace_sptr ws) {
  // Remove the workspace on the main GUI thread
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
void PreviewPlot::handleReplaceEvent(
    WorkspaceAfterReplaceNotification_ptr pNf) {
  MatrixWorkspace_sptr ws =
      boost::dynamic_pointer_cast<MatrixWorkspace>(pNf->object());

  // Ignore non matrix worksapces
  if (!ws)
    return;

  if (getCurvesForWorkspace(ws).size() > 0)
    emit needToHardReplot();
}

/**
 * Creates a new curve and adds it to the plot.
 *
 * @param curveConfig Curve configuration to add to
 * @param ws Workspace pointer
 * @param wsIndex Index of histogram to plot
 * @param curveColour Colour of curve
 */
void PreviewPlot::addCurve(PlotCurveConfiguration &curveConfig,
                           MatrixWorkspace_sptr ws, const size_t wsIndex,
                           const QColor &curveColour,
                           const QString &curveName) {
  // Check the workspace index is in range
  if (wsIndex >= ws->getNumberHistograms())
    throw std::runtime_error("Workspace index is out of range, cannot plot.");

  // Check the X axis is large enough
  if (ws->x(0).size() < 2)
    throw std::runtime_error(
        "X axis is too small to generate a histogram plot.");

  // Convert X axis to squared if needed
  if (getAxisType(QwtPlot::xBottom) == "Squared") {
    Mantid::API::IAlgorithm_sptr convertXAlg =
        Mantid::API::AlgorithmManager::Instance().create(
            "ConvertAxisByFormula");
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

  auto wsDataY = ws->y(wsIndex);

  // If using log scale need to remove all negative Y values
  bool logYScale = getAxisType(QwtPlot::yLeft) == "Logarithmic";
  if (logYScale) {
    // Remove negative data in order to search for minimum positive value
    std::vector<double> validData(wsDataY.size());
    auto it = std::remove_copy_if(wsDataY.begin(), wsDataY.end(),
                                  validData.begin(), isNegative);
    validData.resize(std::distance(validData.begin(), it));

    // Get minimum positive value
    double minY = *std::min_element(validData.begin(), validData.end());

    // Set all negative values to minimum positive value
    std::replace_if(wsDataY.begin(), wsDataY.end(), isNegative, minY);
  }

  // Create the Qwt data
  const auto &wsXPoints = ws->points(wsIndex);

  QwtArray<double> dataX = QVector<double>::fromStdVector(wsXPoints.rawData());
  QwtArray<double> dataY = QVector<double>::fromStdVector(wsDataY.rawData());
  QwtArrayData wsData(dataX, dataY);

  auto curveStyle = QwtPlotCurve::Lines;
  if (m_curveStyle.contains(curveName))
    curveStyle = m_curveStyle[curveName];

  auto curveSymbol = QwtSymbol::NoSymbol;
  if (m_curveSymbol.contains(curveName))
    curveSymbol = m_curveSymbol[curveName];

  // Create the new curve
  curveConfig.curve = new QwtPlotCurve();
  curveConfig.curve->setStyle(curveStyle);
  curveConfig.curve->setSymbol(
      QwtSymbol(curveSymbol, QBrush(), QPen(), QSize(7, 7)));
  curveConfig.curve->setData(wsData);
  curveConfig.curve->setPen(curveColour);
  curveConfig.curve->attach(m_uiForm.plot);

  // Create error bars if needed
  if (curveConfig.showErrorsAction->isChecked()) {
    curveConfig.errorCurve =
        new ErrorCurve(curveConfig.curve, ws->e(wsIndex).rawData());
    curveConfig.errorCurve->attach(m_uiForm.plot);
  } else {
    curveConfig.errorCurve = nullptr;
  }
}

/**
 * Sets the style of the curve.
 *
 * @param curveName The name of the Curve
 * @param style The style of the curve
 */
void PreviewPlot::setCurveStyle(const QString &curveName, const int style) {
  m_curveStyle[curveName] = QwtPlotCurve::CurveStyle(style);
}

/**
 * Sets the style of the curve.
 *
 * @param curveName The name of the Curve
 * @param symbol The symbol for each data point
 */
void PreviewPlot::setCurveSymbol(const QString &curveName, const int symbol) {
  m_curveSymbol[curveName] = QwtSymbol::Style(symbol);
}

/**
 * Removes a curve from the plot.
 *
 * @param curve Curve to remove
 */
void PreviewPlot::removeCurve(QwtPlotItem *curve) {
  if (!curve)
    return;

  // Take it off the plot
  curve->attach(nullptr);

  // Delete it
  delete curve;
  curve = nullptr;
}

/**
 * Helper function for adding a set of items to an exclusive menu on the context
 *menu.
 *
 * @param menuName Name of sub menu
 * @param group Pointer to ActionGroup
 * @param items List of item names
 * @param defaultItem Default item name
 * @return List of Actions added
 */
QList<QAction *> PreviewPlot::addOptionsToMenus(QString menuName,
                                                QActionGroup *group,
                                                QStringList items,
                                                QString defaultItem) {
  QMenu *menu = new QMenu(m_contextMenu);

  for (auto &item : items) {
    QAction *action = new QAction(item, menu);
    action->setCheckable(true);

    // Add to the menu and action group
    group->addAction(action);
    menu->addAction(action);

    // Select default
    action->setChecked(item == defaultItem);
  }

  QAction *menuAction = new QAction(menuName, menu);
  menuAction->setMenu(menu);
  m_contextMenu->addAction(menuAction);

  return group->actions();
}

/**
 * Returns the type of axis scale specified for a given axis.
 *
 * @param axisID ID of axis
 * @return Axis type as string
 */
QString PreviewPlot::getAxisType(int axisID) {
  QString axisType("Linear");
  QAction *selectedAxisType = nullptr;

  if (axisID == QwtPlot::xBottom)
    selectedAxisType = m_xAxisTypeGroup->checkedAction();
  else if (axisID == QwtPlot::yLeft)
    selectedAxisType = m_yAxisTypeGroup->checkedAction();
  else
    return QString();

  if (selectedAxisType)
    axisType = selectedAxisType->text();

  return axisType;
}

/**
 * Gets a list of curve names that are plotted form the given workspace.
 *
 * @param ws Pointer to workspace
 * @return List of curve names
 */
QStringList PreviewPlot::getCurvesForWorkspace(const MatrixWorkspace_sptr ws) {
  QStringList curveNames;

  for (auto it = m_curves.begin(); it != m_curves.end(); ++it) {
    if (it.value().ws == ws)
      curveNames << it.key();
  }

  return curveNames;
}

/**
 * Handles displaying the context menu when a user right clicks on the plot.
 *
 * @param position Position at which to show menu
 */
void PreviewPlot::showContextMenu(QPoint position) {
  // Show the context menu
  m_contextMenu->popup(m_uiForm.plot->mapToGlobal(position));
}

/**
 * Handles the view tool being selected from the context menu.
 */
void PreviewPlot::handleViewToolSelect() {
  QAction *selectedPlotType = m_plotToolGroup->checkedAction();
  if (!selectedPlotType)
    return;

  QString selectedTool = selectedPlotType->text();
  if (selectedTool == "None") {
    togglePanTool(false);
    toggleZoomTool(false);
  } else if (selectedTool == "Pan") {
    togglePanTool(true);
  } else if (selectedTool == "Zoom") {
    toggleZoomTool(true);
  }
}

/**
 * Handles a change in the plot axis type.
 */
void PreviewPlot::handleAxisTypeSelect() {
  // Determine the type of engine to use for each axis
  QString xAxisType = getAxisType(QwtPlot::xBottom);
  QString yAxisType = getAxisType(QwtPlot::yLeft);

  QwtScaleEngine *xEngine = nullptr;
  QwtScaleEngine *yEngine = nullptr;

  // Get the X axis engine
  if (xAxisType == "Linear") {
    xEngine = new QwtLinearScaleEngine();
  } else if (xAxisType == "Logarithmic") {
    xEngine = new QwtLog10ScaleEngine();
  } else if (xAxisType == "Squared") {
    xEngine = new QwtLinearScaleEngine();
  }

  // Get the Y axis engine
  if (yAxisType == "Linear") {
    yEngine = new QwtLinearScaleEngine();
  } else if (yAxisType == "Logarithmic") {
    yEngine = new QwtLog10ScaleEngine();
  }

  // Set the axis scale engines
  if (xEngine)
    m_uiForm.plot->setAxisScaleEngine(QwtPlot::xBottom, xEngine);

  if (yEngine)
    m_uiForm.plot->setAxisScaleEngine(QwtPlot::yLeft, yEngine);

  emit axisScaleChanged();

  // Hide range selectors on X axis when X axis scale is X^2
  bool xIsSquared = xAxisType == "Squared";
  for (auto it = m_rangeSelectors.begin(); it != m_rangeSelectors.end(); ++it) {
    const QString &rsName = it.key();
    RangeSelector *rs = it.value();
    RangeSelector::SelectType type = rs->getType();

    if (type == RangeSelector::XMINMAX || type == RangeSelector::XSINGLE) {
      // When setting to invisible save the last visibility setting
      if (xIsSquared) {
        m_rsVisibility[rsName] = rs->isVisible();
        rs->setVisible(false);
      } else {
        rs->setVisible(m_rsVisibility[rsName]);
      }
    }
  }

  // Update the plot
  emit needToHardReplot();
}
