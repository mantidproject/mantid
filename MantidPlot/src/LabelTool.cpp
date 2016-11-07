#include "LabelTool.h"
// MantidPlot
#include "Graph.h"
#include "LegendWidget.h"
#include "Mantid/MantidMatrixCurve.h"
#include "TextDialog.h"
#include "Mantid/LabelToolLogValuesDialog.h"
// Mantid
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/Run.h"
// Qwt
#include <qwt_picker.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_widget.h>
// Qt
#include <QMenu>

LabelTool::LabelTool(Graph *graph)
    : QObject(graph->plotWidget()->canvas()), PlotToolInterface(graph),

      m_canvasPicker(new QwtPicker(graph->plotWidget()->canvas())),
      m_xAxisPicker(
          new QwtPicker(graph->plotWidget()->axisWidget(Plot::xBottom))),
      m_yAxisPicker(
          new QwtPicker(graph->plotWidget()->axisWidget(Plot::yLeft))),
      m_xPos(), m_yPos(), m_xPosSigFigs(), m_yPosSigFigs(), m_error(),
      m_dataCoords(), m_curveWsName() {
  connect(m_xAxisPicker, SIGNAL(selected(const QwtPolygon &)), this,
          SLOT(xAxisClicked(const QwtPolygon &)));
  connect(m_yAxisPicker, SIGNAL(selected(const QwtPolygon &)), this,
          SLOT(yAxisClicked(const QwtPolygon &)));
  connect(m_canvasPicker, SIGNAL(selected(const QwtPolygon &)), this,
          SLOT(graphAreaClicked(const QwtPolygon &)));

  m_xAxisPicker->setSelectionFlags(QwtPicker::PointSelection |
                                   QwtPicker::ClickSelection);
  m_yAxisPicker->setSelectionFlags(QwtPicker::PointSelection |
                                   QwtPicker::ClickSelection);
  m_canvasPicker->setSelectionFlags(QwtPicker::PointSelection |
                                    QwtPicker::ClickSelection);
}

/// Destructor for the tool.
LabelTool::~LabelTool() {
  d_graph->plotWidget()->canvas()->unsetCursor();
  d_graph->plotWidget()->replot();
}

/**
 * When x-axis is clicked, the pixel coordinates are converted to graph
 * coordinates, and displayed in a pop-up menu.
 * @param &x :: A reference to a click on the x-axis.
 */
void LabelTool::xAxisClicked(const QwtPolygon &x) {
  populateMantidCurves();

  // Obtains the x value of the pixel coordinate.
  QPoint xx = x.point(0);
  int xPosition = xx.x();

  // Obtains the origins of the graph area and of the axis.
  QPoint canvasOrigin = d_graph->plotWidget()->canvas()->pos();
  int canvasOriginX = canvasOrigin.x();

  QPoint xOrigin = d_graph->plotWidget()->axisWidget(QwtPlot::xBottom)->pos();
  int xAxisOriginXValue = xOrigin.x();

  /**
   * The difference in the origins is calculated then taken into account when
   * converting the pixel coordinates of the
   * axis into graph coordinates.
   */

  int deltaOrigins = canvasOriginX - xAxisOriginXValue;
  int xPositionCorrected = xPosition - deltaOrigins;

  double xPos =
      d_graph->plotWidget()->invTransform(QwtPlot::xBottom, xPositionCorrected);

  if (xPos < 0) {
    return;
  }
  m_xPosSigFigs.setNum(xPos, 'f', 6);

  QMenu *clickMenu = new QMenu(d_graph);

  QAction *addXAxisLabel = new QAction(m_xPosSigFigs, this);
  clickMenu->addAction(addXAxisLabel);
  connect(addXAxisLabel, SIGNAL(triggered()), this, SLOT(insertXCoord()));

  clickMenu->exec(QCursor::pos());
}

/**
 * When y-axis is clicked, the pixel coordinates are converted to graph
 * coordinates, and displayed in a pop-up menu.
 * @param &y :: A reference to a click on the y-axis.
 */
void LabelTool::yAxisClicked(const QwtPolygon &y) {
  populateMantidCurves();

  // Obtains the x value of the pixel coordinate.
  QPoint yy = y.point(0);
  int yPosition = yy.y();

  // Obtains the origins of the graph area and of the axis.
  QPoint canvasOrigin = d_graph->plotWidget()->canvas()->pos();
  int canvasOriginY = canvasOrigin.y();

  QPoint yOrigin = d_graph->plotWidget()->axisWidget(QwtPlot::yLeft)->pos();
  int yAxisOriginYValue = yOrigin.y();

  /**
   * The difference in the origins is calculated then taken into account when
   * converting the pixel coordinates of the
   * axis into graph coordinates.
   */

  int deltaOrigins = canvasOriginY - yAxisOriginYValue;
  int yPositionCorrected = yPosition - deltaOrigins;

  double yPos =
      d_graph->plotWidget()->invTransform(QwtPlot::yLeft, yPositionCorrected);

  if (yPos < 0) {
    return;
  }
  m_yPosSigFigs.setNum(yPos, 'f', 6);

  QMenu *clickMenu = new QMenu(d_graph);

  QAction *addYAxisLabel = new QAction(m_yPosSigFigs, this);
  clickMenu->addAction(addYAxisLabel);
  connect(addYAxisLabel, SIGNAL(triggered()), this, SLOT(insertYCoord()));

  clickMenu->exec(QCursor::pos());
}

/// Determines the number of curves present on the graph.
void LabelTool::populateMantidCurves() {
  int n_curves = d_graph->curves();

  // Determines whether any of the graphs are MantidMatrixCurves (MMCs) and
  // returns their names into a list.
  QList<MantidMatrixCurve *> mantidMatrixCurves;
  m_mantidMatrixCurves.clear();
  for (int i = 0; i <= n_curves; i++) {
    QwtPlotCurve *indexCurve = d_graph->curve(i);
    MantidMatrixCurve *d = dynamic_cast<MantidMatrixCurve *>(indexCurve);

    if (d) {
      m_mantidMatrixCurves.append(d);
    }
  }
}

/**
 * When the graph area is clicked, pixel coordinates are found and used to
 * determine graph coordinates.
 * Determines the number of MMCs from the list in populateMantidCurves().
 * For each MMC, determines whether a click on the graph area is within close
 * proximity to a data point.
 * If it is, the coordinates of the data point are stored for display.
 * @param &c :: A reference to a click on the graph area; it is bound by the x
 * and y-axis.
 */
void LabelTool::graphAreaClicked(const QwtPolygon &c) {
  populateMantidCurves();
  m_dataCoords.clear();

  QPoint cc = c.point(0);
  int xPosition = cc.x();
  int yPosition = cc.y();

  // std::cout << "xPosition: " << xPosition << " " << "yPosition: " <<
  // yPosition << '\n';

  m_xPos = d_graph->plotWidget()->invTransform(QwtPlot::xBottom, xPosition);
  m_yPos = d_graph->plotWidget()->invTransform(QwtPlot::yLeft, yPosition);

  // Sets the tolerance value, in pixels, for the range in which to search for a
  // data point.
  int tolerance = 7;

  foreach (MantidMatrixCurve *mantidMatrixCurve, m_mantidMatrixCurves) {
    // Sets the upper and lower limits in pixel coordinates for the x and
    // y-axes.
    int upperLimitX = xPosition + tolerance;
    int lowerLimitX = xPosition - tolerance;
    int upperLimitY = yPosition + tolerance;
    int lowerLimitY = yPosition - tolerance;

    /*
    // Gets the workspace name for which the curve belongs to.
    QString workspaceNameOfCurve = mantidMatrixCurve->workspaceName();
    */

    // Gets the number of data points on the curve.
    auto *mwd = mantidMatrixCurve->mantidData();
    size_t numberOfDataPoints = mwd->size();

    /**
    * Gets the pixel value of the x-coordinate value of each data point within
    * range.
    * Of the data points within range along x-axis, finds out if the pixel
    * values of their y-coordinates are within range.
    * If they are, their position within the iteration - i - is stored in a set.
    */

    QSet<size_t> pointsWithinRange;

    for (size_t i = 0; i < numberOfDataPoints; ++i) {
      // The pixel values of the x and y coordinates of the data points.
      int iPixelValueX =
          d_graph->plotWidget()->transform(QwtPlot::xBottom, mwd->x(i));
      int iPixelvalueY =
          d_graph->plotWidget()->transform(QwtPlot::yLeft, mwd->y(i));

      // Comparing the point of clicking with the positioning of the data
      // points.
      if (((iPixelValueX <= upperLimitX) && (iPixelValueX >= lowerLimitX)) &&
          ((iPixelvalueY <= upperLimitY) && (iPixelvalueY >= lowerLimitY))) {
        pointsWithinRange.insert(i);
      }
    }

    // If there are points within the specified ranges.
    if (!pointsWithinRange.isEmpty()) {
      /**
      * Uses Pythagoras' theorem to calculate the distance between the position
      * of a click and
      * the surrounding data points that lie within both the x and y ranges.
      */

      QMap<double, size_t> map;

      foreach (size_t i, pointsWithinRange) {
        double deltaX = m_xPos - mwd->x(i);
        double deltaY = m_yPos - mwd->y(i);
        double deltaXSquared = deltaX * deltaX;
        double deltaYSquared = deltaY * deltaY;

        double distance = sqrt(deltaXSquared + deltaYSquared);

        map[distance] = i;
      }

      // Distances are stored in a list in ascending order.
      QList<double> distances = map.keys();

      // The first element in the list therefore has the shortest distance.
      double shortestDistance = distances[0];
      size_t nearestPointIndex = map[shortestDistance];

      // Obtains the x and y coordinates of the closest data point, ready to be
      // displayed.
      double nearestPointXCoord = mwd->x(nearestPointIndex);
      double nearestPointYCoord = mwd->y(nearestPointIndex);
      double errorOfNearestPoint = mwd->e(nearestPointIndex);
      m_xPosSigFigs.setNum(nearestPointXCoord, 'f', 6);
      m_yPosSigFigs.setNum(nearestPointYCoord, 'f', 6);
      QString errorSigFigs;
      errorSigFigs.setNum(errorOfNearestPoint, 'f', 6);
      m_dataCoords = "(" + m_xPosSigFigs + ", " + m_yPosSigFigs + ")";
      m_error = m_yPosSigFigs + "+/-" + errorSigFigs;

      // Gets the workspace name for which the curve with the datapoint belongs
      // to.
      QString workspaceNameOfCurve = mantidMatrixCurve->workspaceName();
      m_curveWsName = workspaceNameOfCurve;
    }
  } // foreach

  if (!m_dataCoords.size()) {
    // m_dataCoords has not been initialized
    // this means that there are no points near the clicked region
    blankRegionClicked();
  } else {
    // at least one point was close enough to the clicked region
    dataPointClicked();
  }
}

/**
 * Pops up a menu when a click on the graph area is not within close proximity
 * to a data point.
 * Has option to add a label. Also displays info about the workspace.
 * One can also view the log values present.
 */
void LabelTool::blankRegionClicked() {
  QMenu *clickMenu = new QMenu(d_graph);

  // For adding labels onto workspace.
  QAction *addLabel = new QAction(tr("Add a label"), this);
  clickMenu->addAction(addLabel);
  connect(addLabel, SIGNAL(triggered()), this, SLOT(insertTextBox()));

  // For workspace information.
  QMenu *info = clickMenu->addMenu(tr("More info..."));
  QMenu *workspaces = info->addMenu(tr("Workspaces"));

  foreach (QString wsName, workspaceNames()) {
    QAction *qa = new QAction(wsName, this);
    workspaces->addAction(qa);
    connect(qa, SIGNAL(triggered()), this, SLOT(insertLegend()));
  }

  // For viewing log values.
  // For viewing log values.
  QMenu *logVals = info->addMenu(tr("Log values"));

  foreach (QString wsName, workspaceNames()) {
    QAction *qa = new QAction(wsName, this);
    logVals->addAction(qa);
    connect(qa, SIGNAL(triggered()), this, SLOT(showLogValuesDialog()));
  }

  clickMenu->exec(QCursor::pos());
}

/// If the click is within close proximity of a data point, then a different
/// menu is displayed.
void LabelTool::dataPointClicked() {
  QMenu *clickMenu = new QMenu(d_graph);

  // For displaying data coordinates.

  QAction *addCoordinateLabel = new QAction(m_dataCoords, this);
  clickMenu->addAction(addCoordinateLabel);
  connect(addCoordinateLabel, SIGNAL(triggered()), this,
          SLOT(insertDataCoord()));

  QAction *addErrorLabel = new QAction(m_error, this);
  clickMenu->addAction(addErrorLabel);
  connect(addErrorLabel, SIGNAL(triggered()), this, SLOT(insertErrorValue()));

  // add a separator before addErrorLabel
  clickMenu->insertSeparator(addErrorLabel);

  // For adding labels onto workspace.
  QAction *addLabel = new QAction(tr("Add a label"), this);
  clickMenu->addAction(addLabel);
  connect(addLabel, SIGNAL(triggered()), this, SLOT(insertTextBox()));

  // add a separator before addLabel
  clickMenu->insertSeparator(addLabel);

  // For workspace information.
  QMenu *info = clickMenu->addMenu(tr("More info..."));
  QMenu *workspaces = info->addMenu(tr("Workspaces"));

  foreach (QString wsName, workspaceNames()) {
    QAction *qa = new QAction(wsName, this);
    workspaces->addAction(qa);
    connect(qa, SIGNAL(triggered()), this, SLOT(insertLegend()));
  }

  // For viewing log values.
  QMenu *logVals = info->addMenu(tr("Log values"));

  foreach (QString wsName, workspaceNames()) {
    QAction *qa = new QAction(wsName, this);
    logVals->addAction(qa);
    connect(qa, SIGNAL(triggered()), this, SLOT(showLogValuesDialog()));
  }

  clickMenu->exec(QCursor::pos());
}

void LabelTool::showLogValuesDialog() {
  QAction *action = qobject_cast<QAction *>(sender());
  QString wsname = action->text();

  LegendWidget *label = new LegendWidget(d_graph->plotWidget());
  label->setOriginCoord(m_xPos, m_yPos);
  label->setFont(d_graph->axisFont(0));

  LabelToolLogValuesDialog *logValues =
      new LabelToolLogValuesDialog(wsname, label);
  // window set visibe
  logValues->setVisible(true);
}
/// Creates a label with size equal to the axisFont size
void LabelTool::insertLegend() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    LegendWidget *label = new LegendWidget(d_graph->plotWidget());
    label->setOriginCoord(m_xPos, m_yPos);
    label->setFont(d_graph->axisFont(0));
    label->setText(action->text());
  }
}

/// Displays a dialog box to input the contents of a label, then creates the
/// label.
void LabelTool::insertTextBox() {
  TextDialog *textDialog = new TextDialog(TextDialog::TextMarker);
  LegendWidget *label = new LegendWidget(d_graph->plotWidget());

  textDialog->setLegendWidget(label);
  label->setOriginCoord(m_xPos, m_yPos);

  textDialog->exec();
}

/// Removes the active label on the instructions of the user.
void LabelTool::removeTextBox() { d_graph->removeMarker(); }

/**
 * Gets the names of the workspaces for display in the menu when a click is made
 * whilst using the label tool.
 * @return A set of the names of workspaces on the graph.
 */
QSet<QString> LabelTool::workspaceNames() {
  QSet<QString> workspaceNames;

  foreach (MantidMatrixCurve *mantidMatrixCurve, m_mantidMatrixCurves) {
    workspaceNames.insert(mantidMatrixCurve->workspaceName());
  }

  return workspaceNames;
}

/**
 * Gets the log values for display in the menu when a click is made whilst using
 * the label tool.
 * Accounts for the fact that for each workspace, the properties available may
 * differ.
 * @return A set of log properties for each workspace.
 */
QSet<QString> LabelTool::logValues() {
  using Mantid::API::AnalysisDataService;
  using Mantid::API::MatrixWorkspace;
  using Mantid::Kernel::TimeSeriesProperty;

  QSet<QString> logProperties;
  const auto &ads = AnalysisDataService::Instance();
  foreach (QString workspaceName, workspaceNames()) {
    auto matrixWs =
        ads.retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
    const auto &properties = matrixWs->run().getProperties();

    for (const auto &prop : properties) {
      auto timeSeriesProp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
      QString logValue("None");
      if (timeSeriesProp) {
        logValue.setNum(timeSeriesProp->getStatistics().median, 'f', 6);
      } else {
        logValue = QString::fromStdString(prop->value());
      }
      logProperties.insert(QString::fromStdString(prop->name()) + " : " +
                           logValue);
    }
  }
  return logProperties;
}

/// Sets the coordinates for where the label showing the x-position value is
/// to
/// be located.
void LabelTool::insertXCoord() {
  LegendWidget *xCoordLabel = new LegendWidget(d_graph->plotWidget());

  // Calculates the value, in pixel coordinates, where the y-axis intersects
  // the
  // x-axis.
  int yAxisOriginInPixCoords =
      d_graph->plotWidget()->transform(QwtPlot::yLeft, 0.0);

  double yAxisLabelPosition = d_graph->plotWidget()->invTransform(
      QwtPlot::yLeft, (yAxisOriginInPixCoords - 30));
  double xPosSigFigs = m_xPosSigFigs.toDouble();

  xCoordLabel->setOriginCoord(xPosSigFigs, yAxisLabelPosition);
  xCoordLabel->setText(m_xPosSigFigs);
}

/// Sets the coordinates for where the label showing the y-position value is
/// to
/// be located.
void LabelTool::insertYCoord() {
  LegendWidget *yCoordLabel = new LegendWidget(d_graph->plotWidget());

  // Calculates the value, in pixel coordinates, where the x-axis intersects
  // the
  // y-axis.
  int xAxisOriginInPixCoords =
      d_graph->plotWidget()->transform(QwtPlot::xBottom, 0.0);

  double xAxisLabelPosition = d_graph->plotWidget()->invTransform(
      QwtPlot::xBottom, (xAxisOriginInPixCoords + 2));
  double yPosSigFigs = m_yPosSigFigs.toDouble();

  yCoordLabel->setOriginCoord(xAxisLabelPosition, yPosSigFigs);
  yCoordLabel->setText(m_yPosSigFigs);
}

/// Sets the coordinates for where the label showing the coordinates of the
/// data
/// point is to be located.
void LabelTool::insertDataCoord() {
  LegendWidget *dataPointLabel = new LegendWidget(d_graph->plotWidget());

  // x and y pixel coordinates.
  double xGraphCoordOfDataPoint = m_xPosSigFigs.toDouble();
  double yGraphCoordOfDataPoint = m_yPosSigFigs.toDouble();

  int xPixCoordOfDataPoint = d_graph->plotWidget()->transform(
      QwtPlot::xBottom, xGraphCoordOfDataPoint);
  int yPixCoordOfDataPoint =
      d_graph->plotWidget()->transform(QwtPlot::yLeft, yGraphCoordOfDataPoint);

  // The value to shift the x-coordinate of the label by to avoid an overlap
  // with its corresponding data point.
  int shiftValueX = 10;

  double labelCoordinateX = d_graph->plotWidget()->invTransform(
      QwtPlot::xBottom, (xPixCoordOfDataPoint + shiftValueX));
  int xAxisOriginInPixCoords =
      d_graph->plotWidget()->transform(QwtPlot::yLeft, 0.0);

  double labelCoordinateY;

  // Minimum pixel difference allowed between the y-coordinate of click and
  // the
  // x-axis.
  int minDistFromAxis = 25;

  if ((xAxisOriginInPixCoords - yPixCoordOfDataPoint) < minDistFromAxis) {
    int deltaPositionFromAxis = xAxisOriginInPixCoords - yPixCoordOfDataPoint;
    int shiftValueY = minDistFromAxis - deltaPositionFromAxis;

    // Remember, pixel values in the y direction increase downward from the
    // top
    // of the screen.
    int labelYPixCoord = yPixCoordOfDataPoint - shiftValueY;

    labelCoordinateY =
        d_graph->plotWidget()->invTransform(QwtPlot::yLeft, labelYPixCoord);
  }

  else {
    labelCoordinateY = d_graph->plotWidget()->invTransform(
        QwtPlot::yLeft, (yPixCoordOfDataPoint));
  }

  dataPointLabel->setOriginCoord(labelCoordinateX, labelCoordinateY);
  dataPointLabel->setText(m_dataCoords);
}

/// Attaches a label close to a data point, showing the error associated with
/// the y-value.
void LabelTool::insertErrorValue() {
  LegendWidget *errorPointLabel = new LegendWidget(d_graph->plotWidget());

  // x and y pixel coordinates.
  double xGraphCoordOfDataPoint = m_xPosSigFigs.toDouble();
  double yGraphCoordOfDataPoint = m_yPosSigFigs.toDouble();

  int xPixCoordOfDataPoint = d_graph->plotWidget()->transform(
      QwtPlot::xBottom, xGraphCoordOfDataPoint);
  int yPixCoordOfDataPoint =
      d_graph->plotWidget()->transform(QwtPlot::yLeft, yGraphCoordOfDataPoint);

  // The value to shift the x-coordinate of the label by to avoid an overlap
  // with its corresponding data point.
  int shiftValueX = 10;

  double labelCoordinateX = d_graph->plotWidget()->invTransform(
      QwtPlot::xBottom, (xPixCoordOfDataPoint + shiftValueX));
  int xAxisOriginInPixCoords =
      d_graph->plotWidget()->transform(QwtPlot::yLeft, 0.0);

  double labelCoordinateY;

  // Minimum pixel difference allowed between the y-coordinate of click and
  // the
  // x-axis.
  int minDistFromAxis = 25;

  if ((xAxisOriginInPixCoords - yPixCoordOfDataPoint) < minDistFromAxis) {
    int deltaPositionFromAxis = xAxisOriginInPixCoords - yPixCoordOfDataPoint;
    int shiftValueY = minDistFromAxis - deltaPositionFromAxis;

    // Remember, pixel values in the y direction increase downward from the
    // top
    // of the screen.
    int labelYPixCoord = yPixCoordOfDataPoint - shiftValueY;

    labelCoordinateY =
        d_graph->plotWidget()->invTransform(QwtPlot::yLeft, labelYPixCoord);
  }

  else {
    labelCoordinateY = d_graph->plotWidget()->invTransform(
        QwtPlot::yLeft, (yPixCoordOfDataPoint));
  }

  errorPointLabel->setOriginCoord(labelCoordinateX, labelCoordinateY);
  errorPointLabel->setText(m_error);
}
