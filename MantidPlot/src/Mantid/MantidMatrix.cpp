// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMatrix.h"
#include "../ApplicationWindow.h"
#include "../Graph3D.h"
#include "../Spectrogram.h"
#include "MantidKernel/Logger.h"
#include "MantidMatrixDialog.h"
#include "MantidMatrixFunction.h"
#include "MantidMatrixModel.h"
#include "MantidUI.h"
#include "Preferences.h"
#include <MantidQtWidgets/Common/pixmaps.h>

#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidQtWidgets/Common/PlotAxis.h"

#include <QApplication>
#include <QClipboard>

#include <QScrollBar>

#include <algorithm>
#include <cmath>
#include <limits>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::API;
using namespace Mantid::Geometry;

// Register the window into the WindowFactory
DECLARE_WINDOW(MantidMatrix)

namespace {
Logger g_log("MantidMatrix");
}

namespace {
/**
 * Converts an integer value to the corrsponding enum
 * @param i: the integer to check
 * @returns the corresponding model type
 */
MantidMatrixModel::Type intToModelType(int i) {
  switch (i) {
  case 0:
    return MantidMatrixModel::Y;
  case 1:
    return MantidMatrixModel::X;
  case 2:
    return MantidMatrixModel::E;
  case 3:
    return MantidMatrixModel::DX;
  default:
    g_log.error("Trying to convert to an unknown MantidMatrixModel type. "
                "Defaulting to Y type.");
    return MantidMatrixModel::Y;
  }
}

/**
 * Converts an model enum into a corresponding integer
 * @parm type: the model type to check
 * @returns the corresponding integer value
 */
int modelTypeToInt(MantidMatrixModel::Type type) {
  switch (type) {
  case MantidMatrixModel::Y:
    return 0;
  case MantidMatrixModel::X:
    return 1;
  case MantidMatrixModel::E:
    return 2;
  case MantidMatrixModel::DX:
    return 3;
  default:
    g_log.error("Trying to convert to an unknown MantidMatrixModel to an "
                "integer. Defaulting to 0.");
    return 0;
  }
}
} // namespace

MantidMatrix::MantidMatrix(Mantid::API::MatrixWorkspace_const_sptr ws,
                           QWidget *parent, const QString &label,
                           const QString &name, int start, int end)
    : MdiSubWindow(parent, label, name, nullptr), WorkspaceObserver(),
      m_workspace(ws), y_start(0.0), y_end(0.0), m_histogram(false), m_min(0),
      m_max(0), m_are_min_max_set(false), m_boundingRect(),
      m_strName(name.toStdString()), m_selectedRows(), m_selectedCols() {
  m_workspace = ws;

  setup(ws, start, end);
  setWindowTitle(name);
  setName(name);

  m_modelY = new MantidMatrixModel(this, ws.get(), m_rows, m_cols, m_startRow,
                                   MantidMatrixModel::Y);
  m_table_viewY = new QTableView();
  connectTableView(m_table_viewY, m_modelY);
  setColumnsWidth(0, MantidPreferences::MantidMatrixColumnWidthY());
  setNumberFormat(0, MantidPreferences::MantidMatrixNumberFormatY(),
                  MantidPreferences::MantidMatrixNumberPrecisionY());

  m_modelX = new MantidMatrixModel(this, ws.get(), m_rows, m_cols, m_startRow,
                                   MantidMatrixModel::X);
  m_table_viewX = new QTableView();
  connectTableView(m_table_viewX, m_modelX);
  setColumnsWidth(1, MantidPreferences::MantidMatrixColumnWidthX());
  setNumberFormat(1, MantidPreferences::MantidMatrixNumberFormatX(),
                  MantidPreferences::MantidMatrixNumberPrecisionX());

  m_modelE = new MantidMatrixModel(this, ws.get(), m_rows, m_cols, m_startRow,
                                   MantidMatrixModel::E);
  m_table_viewE = new QTableView();
  connectTableView(m_table_viewE, m_modelE);
  setColumnsWidth(2, MantidPreferences::MantidMatrixColumnWidthE());
  setNumberFormat(2, MantidPreferences::MantidMatrixNumberFormatE(),
                  MantidPreferences::MantidMatrixNumberPrecisionE());

  m_YTabLabel = QString("Y values");
  m_XTabLabel = QString("X values");
  m_ETabLabel = QString("Errors");

  m_tabs = new QTabWidget(this);
  m_tabs->insertTab(0, m_table_viewY, m_YTabLabel);
  m_tabs->insertTab(1, m_table_viewX, m_XTabLabel);
  m_tabs->insertTab(2, m_table_viewE, m_ETabLabel);

  setWidget(m_tabs);
  // for synchronizing the views
  // index is zero for the default view
  m_PrevIndex = 0;
  // install event filter on  these objects
  m_table_viewY->installEventFilter(this);
  m_table_viewX->installEventFilter(this);
  m_table_viewE->installEventFilter(this);

  connect(m_tabs, SIGNAL(currentChanged(int)), this, SLOT(viewChanged(int)));

  setGeometry(50, 50,
              qMin(5, numCols()) *
                      m_table_viewY->horizontalHeader()->sectionSize(0) +
                  55,
              (qMin(10, numRows()) + 1) *
                      m_table_viewY->verticalHeader()->sectionSize(0) +
                  100);

  // Add an extension for the DX component if required
  if (ws->hasDx(0)) {
    addMantidMatrixTabExtension(MantidMatrixModel::DX);
  }

  observeAfterReplace();
  observePreDelete();
  observeADSClear();

  connect(this, SIGNAL(needWorkspaceChange(Mantid::API::MatrixWorkspace_sptr)),
          this, SLOT(changeWorkspace(Mantid::API::MatrixWorkspace_sptr)));
  connect(this, SIGNAL(needToClose()), this, SLOT(closeMatrix()));

  connect(this, SIGNAL(closedWindow(MdiSubWindow *)), this,
          SLOT(selfClosed(MdiSubWindow *)));

  confirmClose(false);
}

bool MantidMatrix::eventFilter(QObject *object, QEvent *e) {
  // if it's context menu on any of the views
  if (e->type() == QEvent::ContextMenu &&
      (object == m_table_viewY || object == m_table_viewX ||
       object == m_table_viewE ||
       m_extensionRequest.tableViewMatchesObject(m_extensions, object))) {
    e->accept();
    emit showContextMenu();
    return true;
  }
  return MdiSubWindow::eventFilter(object, e);
}

/** Called when switching between tabs
 *  @param index :: The index of the new active tab
 */
void MantidMatrix::viewChanged(int index) {
  // get the previous view and selection model
  QTableView *prevView = (QTableView *)m_tabs->widget(m_PrevIndex);
  if (prevView) {
    QItemSelectionModel *oldSelModel = prevView->selectionModel();
    QItemSelectionModel *selModel = activeView()->selectionModel();
    // Copy the selection from the previous tab into the newly-activated one
    selModel->select(oldSelModel->selection(), QItemSelectionModel::Select);
    // Clear the selection on the now-hidden tab
    oldSelModel->clearSelection();

    m_PrevIndex = index;
    // get the previous tab scrollbar positions
    int hValue = prevView->horizontalScrollBar()->value();
    int vValue = prevView->verticalScrollBar()->value();
    // to synchronize the views
    // set  the previous view  scrollbar positions to current view
    activeView()->horizontalScrollBar()->setValue(hValue);
    activeView()->verticalScrollBar()->setValue(vValue);
  }
}

void MantidMatrix::setup(Mantid::API::MatrixWorkspace_const_sptr ws, int start,
                         int end) {
  if (!ws) {
    QMessageBox::critical(nullptr, "WorkspaceMatrixModel error",
                          "2D workspace expected.");
    m_rows = 0;
    m_cols = 0;
    m_startRow = 0;
    m_endRow = 0;
    return;
  }

  m_workspace = ws;
  m_workspaceTotalHist = static_cast<int>(ws->getNumberHistograms());
  m_startRow = (start < 0 || start >= m_workspaceTotalHist) ? 0 : start;
  m_endRow = (end < 0 || end >= m_workspaceTotalHist || end < start)
                 ? m_workspaceTotalHist - 1
                 : end;
  m_rows = m_endRow - m_startRow + 1;
  try {
    // let the workspace do its thing
    m_cols = static_cast<int>(ws->blocksize());
  } catch (std::length_error &) {
    // otherwise get the maximum
    m_cols = static_cast<int>(ws->y(0).size());
    for (int i = 0; i < m_workspaceTotalHist; ++i) {
      m_cols = std::max(m_cols, static_cast<int>(ws->y(i).size()));
    }
  }
  if (ws->isHistogramData())
    m_histogram = true;
  connect(this, SIGNAL(needsUpdating()), this, SLOT(repaintAll()));

  m_bk_color = QColor(128, 255, 255);
  m_matrix_icon = getQPixmap("mantid_matrix_xpm");
  m_column_width = 100;
}

void MantidMatrix::connectTableView(QTableView *view,
                                    MantidMatrixModel *model) {
  view->setSizePolicy(
      QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
  view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  view->setModel(model);
  view->setCornerButtonEnabled(false);
  view->setFocusPolicy(Qt::StrongFocus);

  QPalette pal = view->palette();
  pal.setColor(QPalette::Base, m_bk_color);
  view->setPalette(pal);

  // set header properties
  QHeaderView *hHeader = (QHeaderView *)view->horizontalHeader();
  hHeader->setMovable(false);
  hHeader->setResizeMode(QHeaderView::Interactive);
  hHeader->setDefaultSectionSize(m_column_width);

  view->resizeRowToContents(0);
  int row_height = view->rowHeight(0);

  QHeaderView *vHeader = (QHeaderView *)view->verticalHeader();
  vHeader->setDefaultSectionSize(row_height);
  vHeader->setResizeMode(QHeaderView::Fixed);
  vHeader->setMovable(false);
}

double MantidMatrix::cell(int row, int col) { return m_modelY->data(row, col); }

QString MantidMatrix::text(int row, int col) {
  const int precision = 15;
  const char format = 'g';
  return QString::number(activeModel()->data(row, col), format, precision);
}

/** Sets new column width in a table view(s).
@param width :: New column width in pixels. All columns have the same width.
@param all :: If true the change will be applied to all three table views.
*/
void MantidMatrix::setColumnsWidth(int width, bool all) {
  if (all) {
    m_table_viewY->horizontalHeader()->setDefaultSectionSize(width);
    m_table_viewX->horizontalHeader()->setDefaultSectionSize(width);
    m_table_viewE->horizontalHeader()->setDefaultSectionSize(width);
    int cols = numCols();
    for (int i = 0; i < cols; i++) {
      m_table_viewY->setColumnWidth(i, width);
      m_table_viewX->setColumnWidth(i, width);
      m_table_viewE->setColumnWidth(i, width);
    }
    m_extensionRequest.setColumnWidthForAll(m_extensions, width, cols);
    MantidPreferences::MantidMatrixColumnWidth(width);
  } else {
    QTableView *table_view = activeView();
    table_view->horizontalHeader()->setDefaultSectionSize(width);
    int cols = numCols();
    for (int i = 0; i < cols; i++)
      table_view->setColumnWidth(i, width);
    auto currentIndex = m_tabs->currentIndex();
    switch (currentIndex) {
    case 0:
      MantidPreferences::MantidMatrixColumnWidthY(width);
      break;
    case 1:
      MantidPreferences::MantidMatrixColumnWidthX(width);
      break;
    case 2:
      MantidPreferences::MantidMatrixColumnWidthE(width);
      break;
    default:
      m_extensionRequest.setColumnWidthPreference(intToModelType(currentIndex),
                                                  m_extensions, width);
    }
  }

  emit modifiedWindow(this);
}

/**  Sets column width to one table view.
@param i :: ordinal number of the view. 0 - Y, 1 - X, 2 - Error
@param width :: New column width in pixels. All columns have the same width.
*/
void MantidMatrix::setColumnsWidth(int i, int width) {
  QTableView *table_view;
  switch (i) {
  case 0:
    table_view = m_table_viewY;
    MantidPreferences::MantidMatrixColumnWidthY(width);
    break;
  case 1:
    table_view = m_table_viewX;
    MantidPreferences::MantidMatrixColumnWidthX(width);
    break;
  case 2:
    table_view = m_table_viewE;
    MantidPreferences::MantidMatrixColumnWidthE(width);
    break;
  default:
    table_view = m_extensionRequest.getTableView(
        intToModelType(i), m_extensions, width, activeView());
  }

  table_view->horizontalHeader()->setDefaultSectionSize(width);
  int cols = numCols();
  for (int i = 0; i < cols; i++)
    table_view->setColumnWidth(i, width);

  emit modifiedWindow(this);
}

/**  Returns the width of a column.
@param i :: ordinal number of the view. 0 - Y, 1 - X, 2 - Error
@return The column width in pixels. All columns have the same width.
*/
int MantidMatrix::columnsWidth(int i) {
  switch (i) {
  case 0:
    return m_table_viewY->columnWidth(0);
  case 1:
    return m_table_viewX->columnWidth(0);
  case 2:
    return m_table_viewE->columnWidth(0);
  default:
    return m_extensionRequest.getColumnWidth(intToModelType(i), m_extensions,
                                             activeView()->columnWidth(0));
  };
}

/**  Return the pointer to the active table view.
 */
QTableView *MantidMatrix::activeView() {
  auto currentIndex = m_tabs->currentIndex();
  switch (currentIndex) {
  case 0:
    return m_table_viewY;
  case 1:
    return m_table_viewX;
  case 2:
    return m_table_viewE;
  default:
    return m_extensionRequest.getActiveView(intToModelType(currentIndex),
                                            m_extensions, m_table_viewY);
  }
}

/**  Returns the pointer to the active model.
 */
MantidMatrixModel *MantidMatrix::activeModel() {
  auto currentIndex = m_tabs->currentIndex();
  switch (currentIndex) {
  case 0:
    return m_modelY;
  case 1:
    return m_modelX;
  case 2:
    return m_modelE;
  default:
    return m_extensionRequest.getActiveModel(intToModelType(currentIndex),
                                             m_extensions, m_modelY);
  }
}

/**  Copies the current selection in the active table view into the system
 * clipboard.
 */
void MantidMatrix::copySelection() {
  QItemSelectionModel *selModel = activeView()->selectionModel();
  QString s = "";
  QString eol = applicationWindow()->endOfLine();
  if (!selModel->hasSelection()) {
    QModelIndex index = selModel->currentIndex();
    if (!index.isValid()) {
      // No text boxes were selected. So we cannot copy anything
      return;
    }

    s = text(index.row(), index.column());
  } else {
    QItemSelection sel = selModel->selection();
    QListIterator<QItemSelectionRange> it(sel);
    if (!it.hasNext())
      return;

    QItemSelectionRange cur = it.next();
    int top = cur.top();
    int bottom = cur.bottom();
    int left = cur.left();
    int right = cur.right();
    for (int i = top; i <= bottom; i++) {
      for (int j = left; j < right; j++)
        s += text(i, j) + "\t";
      s += text(i, right) + eol;
    }
  }
  // Copy text into the clipboard
  QApplication::clipboard()->setText(s.trimmed());
}

/**  Returns minimum and maximum values in the matrix.
If setRange(...) has not been called it returns the true smallest and largest
Y-values in the matrix,
otherwise the values set with setRange(...) are returned. These are needed in
plotGraph2D to set
the range of the third, colour axis.
@param[out] min is set to the minumium value
@param[out] max is set to the maximum value
*/
void MantidMatrix::range(double *min, double *max) {
  if (!m_are_min_max_set) {
    findYRange(m_workspace, m_min, m_max);
    m_are_min_max_set = true;
  }
  *min = m_min;
  *max = m_max;
}

/**  Sets new minimum and maximum Y-values which can be displayed in a 2D graph
 */
void MantidMatrix::setRange(double min, double max) {
  m_min = min;
  m_max = max;
  m_are_min_max_set = true;
}

double **MantidMatrix::allocateMatrixData(int rows, int columns) {
  double **data = (double **)malloc(rows * sizeof(double *));
  if (!data) {
    QMessageBox::critical(
        nullptr, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
        tr("Not enough memory, operation aborted!"));
    return nullptr;
  }

  for (int i = 0; i < rows; ++i) {
    data[i] = (double *)malloc(columns * sizeof(double));
    if (!data[i]) {
      for (int j = 0; j < i; j++)
        free(data[j]);
      free(data);

      QMessageBox::critical(
          nullptr, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
          tr("Not enough memory, operation aborted!"));
      return nullptr;
    }
  }
  return data;
}

void MantidMatrix::freeMatrixData(double **data, int rows) {
  for (int i = 0; i < rows; i++)
    free(data[i]);

  free(data);
}

void MantidMatrix::goTo(int row, int col) {
  if (row < 1 || row > numRows())
    return;
  if (col < 1 || col > numCols())
    return;

  activeView()->selectionModel()->select(activeModel()->index(row - 1, col - 1),
                                         QItemSelectionModel::ClearAndSelect);
  activeView()->scrollTo(activeModel()->index(row - 1, col - 1),
                         QAbstractItemView::PositionAtTop);
}

void MantidMatrix::goToRow(int row) {
  if (row < 1 || row > numRows())
    return;

  //	activeView()->selectRow(row - 1); //For some reason, this did not
  // highlight the row at all, hence the stupid line below
  activeView()->selectionModel()->select(
      QItemSelection(activeModel()->index(row - 1, 0),
                     activeModel()->index(row - 1, numCols() - 1)),
      QItemSelectionModel::ClearAndSelect);

  activeView()->scrollTo(activeModel()->index(row - 1, 0),
                         QAbstractItemView::PositionAtCenter);
}

void MantidMatrix::goToColumn(int col) {
  if (col < 1 || col > numCols())
    return;

  //	activeView()->selectColumn(col - 1); //For some reason, this did not
  // highlight the row at all, hence the stupid line below
  activeView()->selectionModel()->select(
      QItemSelection(activeModel()->index(0, col - 1),
                     activeModel()->index(numRows() - 1, col - 1)),
      QItemSelectionModel::ClearAndSelect);
  activeView()->scrollTo(activeModel()->index(0, col - 1),
                         QAbstractItemView::PositionAtCenter);
}

double MantidMatrix::dataX(int row, int col) const {
  if (!m_workspace || row >= numRows() || col >= numCols())
    return 0.;
  const auto &x = m_workspace->x(row + m_startRow);
  if (col >= static_cast<int>(x.size()))
    return 0.;
  return x[col];
}

double MantidMatrix::dataY(int row, int col) const {
  if (!m_workspace || row >= numRows() || col >= numCols())
    return 0.;
  const auto &y = m_workspace->y(row + m_startRow);
  if (col >= static_cast<int>(y.size()))
    return 0.;
  return y[col];
}

double MantidMatrix::dataE(int row, int col) const {
  if (!m_workspace || row >= numRows() || col >= numCols())
    return 0.;
  const auto &e = m_workspace->e(row + m_startRow);
  if (col >= static_cast<int>(e.size()))
    return 0.;
  return e[col];
}

double MantidMatrix::dataDx(int row, int col) const {
  if (!m_workspace || row >= numRows() || col >= numCols())
    return 0.;
  const auto &dx = m_workspace->dx(row + m_startRow);
  if (col >= static_cast<int>(dx.size()))
    return 0.;
  return dx[col];
}

QString MantidMatrix::workspaceName() const {
  return QString::fromStdString(m_strName);
}

QwtDoubleRect MantidMatrix::boundingRect() {
  const int defaultNumberSpectroGramRows = 700;
  const int defaultNumberSpectroGramColumns = 700;
  if (m_boundingRect.isNull()) {
    m_spectrogramRows = numRows() > defaultNumberSpectroGramRows
                            ? numRows()
                            : defaultNumberSpectroGramRows;

    // This is only meaningful if a 2D (or greater) workspace
    if (m_workspace->axes() > 1) {
      const Mantid::API::Axis *const ax = m_workspace->getAxis(1);
      y_start = (*ax)(m_startRow);
      y_end = (*ax)(m_endRow);
    }

    double dy = fabs(y_end - y_start) / (double)(numRows() - 1);

    int i0 = m_startRow;
    x_start = x_end = 0;
    while (x_start == x_end && i0 <= m_endRow) {
      const auto &X = m_workspace->x(i0);
      x_start = X[0];
      const size_t y_size = m_workspace->y(i0).size();
      if (X.size() != y_size)
        x_end = X[y_size];
      else
        x_end = X[y_size - 1];
      if (!std::isfinite(x_start) || !std::isfinite(x_end)) {
        x_start = x_end = 0;
      }
      i0++;
    }

    // if i0 > m_endRow there aren't any plottable rows
    if (i0 <= m_endRow) {
      // check if all X vectors are the same
      bool theSame = true;
      double dx = 0.;
      for (int i = i0; i <= m_endRow; ++i) {
        const auto &X = m_workspace->x(i);
        if (X.front() != x_start || X.back() != x_end) {
          theSame = false;
          break;
        }
      }
      dx = fabs(x_end - x_start) / (double)(numCols() - 1);

      if (!theSame) {
        // Find the smallest bin width and thus the number of columns in a
        // spectrogram
        // that can be plotted from this matrix
        double ddx = dx;
        for (int i = m_startRow + 1; i <= m_endRow; ++i) {
          const auto &X = m_workspace->x(i);
          if (X.front() < x_start) {
            double xs = X.front();
            if (!std::isfinite(xs))
              continue;
            x_start = xs;
          }
          if (X.back() > x_end) {
            double xe = X.back();
            if (!std::isfinite(xe))
              continue;
            x_end = xe;
          }
          for (int j = 1; j < static_cast<int>(X.size()); ++j) {
            double d = X[j] - X[j - 1];
            if (ddx == 0 && d < ddx) {
              ddx = d;
            }
          }
        }
        m_spectrogramCols = static_cast<int>((x_end - x_start) / ddx);
        if (m_spectrogramCols < defaultNumberSpectroGramColumns)
          m_spectrogramCols = defaultNumberSpectroGramColumns;
      } else {
        m_spectrogramCols = numCols() > defaultNumberSpectroGramColumns
                                ? numCols()
                                : defaultNumberSpectroGramColumns;
      }
      m_boundingRect =
          QwtDoubleRect(qMin(x_start, x_end) - 0.5 * dx,
                        qMin(y_start, y_end) - 0.5 * dy,
                        fabs(x_end - x_start) + dx, fabs(y_end - y_start) + dy)
              .normalized();

    } else {
      m_spectrogramCols = 0;
      m_boundingRect = QwtDoubleRect(0, qMin(y_start, y_end) - 0.5 * dy, 1,
                                     fabs(y_end - y_start) + dy)
                           .normalized();
    }
  } // Define the spectrogram bounding box
  return m_boundingRect;
}

//----------------------------------------------------------------------------

Graph3D *MantidMatrix::plotGraph3D(int style) {
  QApplication::setOverrideCursor(Qt::WaitCursor);

  ApplicationWindow *a = applicationWindow();
  QString labl = a->generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", a);
  plot->resize(500, 400);
  plot->setWindowTitle(labl);
  plot->setName(labl);
  plot->setTitle(tr("Workspace ") + name());
  a->customPlot3D(plot);
  plot->customPlotStyle(style);
  int resCol = numCols() / 200;
  int resRow = numRows() / 200;
  plot->setResolution(qMax(resCol, resRow));

  double zMin = 1e300;
  double zMax = -1e300;
  for (int i = 0; i < numRows(); i++) {
    for (int j = 0; j < numCols(); j++) {
      double val = cell(i, j);
      if (val < zMin)
        zMin = val;
      if (val > zMax)
        zMax = val;
    }
  }

  // Calculate xStart(), xEnd(), yStart(), yEnd()
  boundingRect();

  MantidMatrixFunction *fun = new MantidMatrixFunction(*this);
  plot->addFunction(fun, xStart(), xEnd(), yStart(), yEnd(), zMin, zMax,
                    numCols(), numRows());

  using MantidQt::API::PlotAxis;
  plot->setXAxisLabel(PlotAxis(*m_workspace, 0).title());
  plot->setYAxisLabel(PlotAxis(*m_workspace, 1).title());
  plot->setZAxisLabel(PlotAxis(false, *m_workspace).title());

  a->initPlot3D(plot);
  // plot->confirmClose(false);
  QApplication::restoreOverrideCursor();

  return plot;
}

void MantidMatrix::attachMultilayer(MultiLayer *ml) {
  m_plots2D << ml;
  connect(ml, SIGNAL(closedWindow(MdiSubWindow *)), this,
          SLOT(dependantClosed(MdiSubWindow *)));
}

/** Creates a MultiLayer graph and plots this MantidMatrix as a Spectrogram.

@param type :: The "curve" type.
@return Pointer to the created graph.
*/
MultiLayer *MantidMatrix::plotGraph2D(GraphOptions::CurveType type) {
  if (numRows() == 1) {
    QMessageBox::critical(nullptr, "MantidPlot - Error",
                          "Cannot plot a workspace with only one spectrum.");
    return nullptr;
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  ApplicationWindow *a = applicationWindow();
  MultiLayer *g = a->multilayerPlot(a->generateUniqueName(tr("Graph")));
  attachMultilayer(g);
  //#799 fix for  multiple dialog creation on double clicking/ on right click
  // menu scale on  2d plot
  //   a->connectMultilayerPlot(g);
  Graph *plot = g->activeGraph();
  plotSpectrogram(plot, a, type, false, nullptr);
  // g->confirmClose(false);
  QApplication::restoreOverrideCursor();
  return g;
}

Spectrogram *MantidMatrix::plotSpectrogram(Graph *plot, ApplicationWindow *app,
                                           GraphOptions::CurveType type,
                                           bool project,
                                           const ProjectData *const prjData) {
  app->setPreferences(plot);

  plot->setTitle(tr("Workspace ") + name());

  using MantidQt::API::PlotAxis;
  plot->setXAxisTitle(PlotAxis(*m_workspace, 0).title());
  plot->setYAxisTitle(PlotAxis(*m_workspace, 1).title());

  // Set the range on the third, colour axis
  double minz, maxz;
  auto fun = new MantidMatrixFunction(*this);
  range(&minz, &maxz);
  Spectrogram *spgrm =
      plot->plotSpectrogram(fun, m_spectrogramRows, m_spectrogramCols,
                            boundingRect(), minz, maxz, type);
  app->setSpectrogramTickStyle(plot);
  if (spgrm) {
    if (project) {
      spgrm->mutableColorMap().loadMap(prjData->getColormapFile());
      spgrm->setCustomColorMap(spgrm->mutableColorMap());
      spgrm->setIntensityChange(prjData->getIntensity());
      if (!prjData->getGrayScale())
        spgrm->setGrayScale();
      if (prjData->getContourMode()) {
        spgrm->setDisplayMode(QwtPlotSpectrogram::ContourMode, true);
        spgrm->showContourLineLabels(true);
      }
      spgrm->setDefaultContourPen(prjData->getDefaultContourPen());
      spgrm->setColorMapPen(false);
      if (prjData->getColorMapPen())
        spgrm->setColorMapPen(true);
      ContourLinesEditor *contourEditor = prjData->getContourLinesEditor();
      if (contourEditor) {
        contourEditor->setSpectrogram(spgrm);
        contourEditor->updateContents();
        contourEditor->updateContourLevels();
      }
    }
  }
  plot->setAutoScale();
  return spgrm;
}

void MantidMatrix::setBinGraph(MultiLayer *ml, Table *t) {
  MantidUI::setUpBinGraph(ml, name(), workspace());
  connect(ml, SIGNAL(closedWindow(MdiSubWindow *)), this,
          SLOT(dependantClosed(MdiSubWindow *)));
  if (t) {
    m_plots1D[ml] = t;
    connect(t, SIGNAL(closedWindow(MdiSubWindow *)), this,
            SLOT(dependantClosed(MdiSubWindow *)));
  } else
    m_plots2D << ml;
}

/// Returns a list of the selected rows
const QList<int> &MantidMatrix::getSelectedRows() const {
  return m_selectedRows;
}

/**
 * Sets the internal cache of selected rows.
 * @return True if rows are selected, false otherwise
 */
bool MantidMatrix::setSelectedRows() {
  QTableView *tv = activeView();
  QItemSelectionModel *selModel = tv->selectionModel();
  if (!selModel)
    return false;

  m_selectedRows.clear();
  const QModelIndexList rows = selModel->selectedRows();
  QModelIndexList::const_iterator it;
  for (it = rows.constBegin(); it != rows.constEnd(); ++it) {
    m_selectedRows.append(it->row() + m_startRow);
  }

  return (!m_selectedRows.empty());
}

/// Returns a list of the selected columns
const QList<int> &MantidMatrix::getSelectedColumns() const {
  return m_selectedCols;
}

/**
 * Sets the internal cache of selected columns.
 * @return True if columns are selected, false otherwise
 */
bool MantidMatrix::setSelectedColumns() {
  QTableView *tv = activeView();
  QItemSelectionModel *selModel = tv->selectionModel();
  if (!selModel || !selModel->hasSelection())
    return false;

  m_selectedCols.clear();
  const QModelIndexList cols = selModel->selectedColumns();
  QModelIndexList::const_iterator it;
  for (it = cols.constBegin(); it != cols.constEnd(); ++it) {
    m_selectedCols.append(it->column());
  }

  return (!m_selectedCols.empty());
}

void MantidMatrix::dependantClosed(MdiSubWindow *w) {
  if (strcmp(w->metaObject()->className(), "Table") == 0) {
    QMap<MultiLayer *, Table *>::iterator itr;
    for (itr = m_plots1D.begin(); itr != m_plots1D.end(); ++itr) {
      if (itr.value() == dynamic_cast<Table *>(w)) {
        m_plots1D.erase(itr);
        break;
      }
    }
  } else if (strcmp(w->metaObject()->className(), "MultiLayer") == 0) {
    int i = m_plots2D.indexOf(dynamic_cast<MultiLayer *>(w));
    if (i >= 0)
      m_plots2D.remove(i);
    else {
      QMap<MultiLayer *, Table *>::iterator i =
          m_plots1D.find(dynamic_cast<MultiLayer *>(w));
      if (i != m_plots1D.end()) {
        if (i.value() != 0) {
          i.value()->confirmClose(false);
          i.value()->close();
        }
        m_plots1D.erase(i);
      }
    }
  }
}

/**
Repaints all 1D and 2D plots attached to this MantidMatrix
*/
void MantidMatrix::repaintAll() {
  repaint();

  // Repaint 2D plots
  QVector<MultiLayer *>::iterator vEnd = m_plots2D.end();
  for (QVector<MultiLayer *>::iterator vItr = m_plots2D.begin(); vItr != vEnd;
       ++vItr) {
    (*vItr)->activeGraph()->replot();
  }

  // Updates the 1D plots by modifying the attached tables
  QMap<MultiLayer *, Table *>::iterator mEnd = m_plots1D.end();
  for (QMap<MultiLayer *, Table *>::iterator mItr = m_plots1D.begin();
       mItr != mEnd; ++mItr) {
    Table *t = mItr.value();
    if (!t)
      continue;
    int charsToRemove = t->name().size() + 1;
    int nTableCols(t->numCols());
    for (int col = 1; col < nTableCols; ++col) {
      QString colName = t->colName(col).remove(0, charsToRemove);
      if (colName.isEmpty())
        break;
      // Need to determine whether the table was created from plotting a
      // spectrum
      // or a time bin. A spectrum has a Y column name YS and a bin YB
      QString ident = colName.left(2);
      colName.remove(0, 2); // This now contains the number in the MantidMatrix
      int matrixNumber = colName.toInt();
      if (matrixNumber < 0)
        break;
      bool errs = (ident[0] == QChar('E'));
      if (ident[1] == QChar('S')) {
        if (matrixNumber >= numRows())
          break;
        int endCount = numCols();
        for (int j = 0; j < endCount; ++j) {
          if (errs)
            t->setCell(j, col, dataE(matrixNumber, j));
          else
            t->setCell(j, col, dataY(matrixNumber, j));
        }
      } else {
        if (matrixNumber >= numCols())
          break;
        int endCount = numRows();
        for (int j = 0; j < endCount; ++j) {
          if (errs)
            t->setCell(j, col, dataE(j, matrixNumber));
          else
            t->setCell(j, col, dataY(j, matrixNumber));
        }
      }
    }
    t->notifyChanges();
  }
}

void MantidMatrix::afterReplaceHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  if (!ws)
    return;
  if (wsName != m_strName) {
    if (ws == m_workspace) // i.e. this is a rename
    {
      m_strName = wsName;
      QString qwsName = QString::fromStdString(wsName);
      setWindowTitle(qwsName);
      setName(qwsName);
      setObjectName(qwsName);
    }
    return;
  }

  Mantid::API::MatrixWorkspace_sptr new_workspace =
      boost::dynamic_pointer_cast<MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(m_strName));

  // If the cast failed (e.g. Matrix2D became a GroupWorkspace) do not try to
  // change the matrix, just close it
  if (new_workspace) {
    emit needWorkspaceChange(new_workspace);
  } else {
    g_log.warning("Workspace type changed. Closing matrix window.");
    emit needToClose();
  }
}

void MantidMatrix::changeWorkspace(Mantid::API::MatrixWorkspace_sptr ws) {
  if (m_workspaceTotalHist != static_cast<int>(ws->getNumberHistograms()) ||
      m_cols != static_cast<int>(ws->blocksize())) {
    closeDependants();
  }

  // Save selection
  QItemSelectionModel *oldSelModel = activeView()->selectionModel();
  QModelIndexList indexList = oldSelModel->selectedIndexes();
  QModelIndex curIndex = activeView()->currentIndex();

  setup(ws, -1, -1);

  m_modelY = new MantidMatrixModel(this, ws.get(), m_rows, m_cols, m_startRow,
                                   MantidMatrixModel::Y);
  connectTableView(m_table_viewY, m_modelY);
  setNumberFormat(0, MantidPreferences::MantidMatrixNumberFormatY(),
                  MantidPreferences::MantidMatrixNumberPrecisionY());

  m_modelX = new MantidMatrixModel(this, ws.get(), m_rows, m_cols, m_startRow,
                                   MantidMatrixModel::X);
  connectTableView(m_table_viewX, m_modelX);
  setNumberFormat(1, MantidPreferences::MantidMatrixNumberFormatX(),
                  MantidPreferences::MantidMatrixNumberPrecisionX());

  m_modelE = new MantidMatrixModel(this, ws.get(), m_rows, m_cols, m_startRow,
                                   MantidMatrixModel::E);
  connectTableView(m_table_viewE, m_modelE);
  setNumberFormat(2, MantidPreferences::MantidMatrixNumberFormatE(),
                  MantidPreferences::MantidMatrixNumberPrecisionE());

  // Update the extensions
  updateExtensions(ws);

  // Restore selection
  activeView()->setCurrentIndex(curIndex);
  if (indexList.size()) {
    QItemSelection sel(indexList.first(), indexList.last());
    QItemSelectionModel *selModel = activeView()->selectionModel();
    selModel->select(sel, QItemSelectionModel::Select);
  }

  invalidateBoundingRect();

  repaintAll();
}

void MantidMatrix::closeDependants() {
  while (m_plots2D.size()) {
    MultiLayer *ml = m_plots2D.front();
    ml->confirmClose(false);
    ml->close(); // this calls slot dependantClosed() which removes the pointer
                 // from m_plots2D
  }

  while (m_plots1D.size()) {
    MultiLayer *ml = m_plots1D.begin().key();
    ml->confirmClose(false);
    ml->close(); // this calls slot dependantClosed() which removes the pointer
                 // from m_plots1D
  }
}

void MantidMatrix::setNumberFormat(const QChar &f, int prec, bool all) {
  if (all) {
    modelY()->setFormat(f, prec);
    modelX()->setFormat(f, prec);
    modelE()->setFormat(f, prec);
    m_extensionRequest.setNumberFormatForAll(m_extensions, f, prec);
    MantidPreferences::MantidMatrixNumberFormat(f);
    MantidPreferences::MantidMatrixNumberPrecision(prec);
  } else {
    activeModel()->setFormat(f, prec);
    auto current_index = m_tabs->currentIndex();
    switch (m_tabs->currentIndex()) {
    case 0:
      MantidPreferences::MantidMatrixNumberFormatY(f);
      MantidPreferences::MantidMatrixNumberPrecisionY(prec);
      break;
    case 1:
      MantidPreferences::MantidMatrixNumberFormatX(f);
      MantidPreferences::MantidMatrixNumberPrecisionX(prec);
      break;
    case 2:
      MantidPreferences::MantidMatrixNumberFormatE(f);
      MantidPreferences::MantidMatrixNumberPrecisionE(prec);
      break;
    default:
      m_extensionRequest.recordFormat(intToModelType(current_index),
                                      m_extensions, f, prec);
      break;
    }
  }
}

void MantidMatrix::setNumberFormat(int i, const QChar &f, int prec, bool all) {
  (void)all; // Avoid unused warning
  switch (i) {
  case 0:
    m_modelY->setFormat(f, prec);
    MantidPreferences::MantidMatrixNumberFormatY(f);
    MantidPreferences::MantidMatrixNumberPrecisionY(prec);
    break;
  case 1:
    m_modelX->setFormat(f, prec);
    MantidPreferences::MantidMatrixNumberFormatX(f);
    MantidPreferences::MantidMatrixNumberPrecisionX(prec);
    break;
  case 2:
    m_modelE->setFormat(f, prec);
    MantidPreferences::MantidMatrixNumberFormatE(f);
    MantidPreferences::MantidMatrixNumberPrecisionE(prec);
    break;
  default:
    m_extensionRequest.setNumberFormat(intToModelType(i), m_extensions, f,
                                       prec);
    break;
  }
}

QChar MantidMatrix::numberFormat() { return activeModel()->format(); }

int MantidMatrix::precision() { return activeModel()->precision(); }

void MantidMatrix::setMatrixProperties() {
  QWidget *parent = parentWidget();
  MantidMatrixDialog dlg(parent);
  dlg.setMatrix(this);
  dlg.exec();
}

void MantidMatrix::preDeleteHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  (void)wsName; // Avoid unused warning
  if (m_workspace.get() == ws.get()) {
    emit needToClose();
  }
}

void MantidMatrix::clearADSHandle() { emit needToClose(); }

void MantidMatrix::closeMatrix() {
  confirmClose(false);
  close();
}

void MantidMatrix::selfClosed(MdiSubWindow *w) {
  (void)w; // Avoid unused warning
  closeDependants();
}

//-------------------------------
// Python API commands
//------------------------------

void MantidMatrix::goToTab(const QString &name) {
  if (m_tabs->tabText(m_tabs->currentIndex()) == name)
    return;

  if (name == m_YTabLabel) {
    m_tabs->setCurrentIndex(0);
  } else if (name == m_XTabLabel) {
    m_tabs->setCurrentIndex(1);
  } else if (name == m_ETabLabel) {
    m_tabs->setCurrentIndex(2);
  } else
    return;
}

/**  returns the workspace name
 */
const std::string &MantidMatrix::getWorkspaceName() { return m_strName; }

void findYRange(MatrixWorkspace_const_sptr ws, double &miny, double &maxy) {
  // this is here to fill m_min and m_max with numbers that aren't nan
  miny = std::numeric_limits<double>::max();
  maxy = std::numeric_limits<double>::lowest();

  if (ws) {

    PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
    for (int wi = 0; wi < static_cast<int>(ws->getNumberHistograms()); wi++) {
      double local_min, local_max;
      const auto &Y = ws->y(wi);

      local_min = std::numeric_limits<double>::max();
      local_max = std::numeric_limits<double>::lowest();

      for (size_t i = 0; i < Y.size(); i++) {
        double aux = Y[i];
        if (!std::isfinite(aux))
          continue;
        if (aux < local_min)
          local_min = aux;
        if (aux > local_max)
          local_max = aux;
      }

      // Now merge back the local min max
      PARALLEL_CRITICAL(MantidMatrix_range_max) {
        if (local_max > maxy)
          maxy = local_max;
      }
      PARALLEL_CRITICAL(MantidMatrix_range_min) {
        if (local_min < miny)
          miny = local_min;
      }
    }
  }

  // Make up some reasonable values if nothing was found
  if (miny == std::numeric_limits<double>::max())
    miny = 0;
  if (maxy == std::numeric_limits<double>::lowest())
    maxy = miny + 1e6;

  if (maxy == miny) {
    if (maxy == 0.0)
      maxy += 1.0;
    else
      maxy += fabs(miny);
  }
}

MantidQt::API::IProjectSerialisable *
MantidMatrix::loadFromProject(const std::string &lines, ApplicationWindow *app,
                              const int fileVersion) {
  Q_UNUSED(fileVersion);
  TSVSerialiser tsv(lines);

  MantidMatrix *matrix = nullptr;
  if (tsv.selectLine("WorkspaceName")) {
    const std::string wsName = tsv.asString(1);
    MatrixWorkspace_sptr ws;

    if (AnalysisDataService::Instance().doesExist(wsName))
      ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    if (!ws)
      return nullptr;

    matrix = new MantidMatrix(ws, app, "Mantid", QString::fromStdString(wsName),
                              -1, -1);
  }

  if (!matrix)
    return nullptr;

  // Append to the list of mantid matrix apps
  app->addMantidMatrixWindow(matrix);
  app->addMdiSubWindow(matrix);

  if (tsv.selectLine("geometry")) {
    const std::string geometry = tsv.lineAsString("geometry");
    app->restoreWindowGeometry(app, matrix, QString::fromStdString(geometry));
  }

  if (tsv.selectLine("tgeometry")) {
    const std::string geometry = tsv.lineAsString("tgeometry");
    app->restoreWindowGeometry(app, matrix, QString::fromStdString(geometry));
  }

  if (tsv.selectLine("SelectedTab")) {
    int index;
    tsv >> index;
    matrix->m_tabs->setCurrentIndex(index);
  }

  return matrix;
}

std::string MantidMatrix::saveToProject(ApplicationWindow *app) {
  TSVSerialiser tsv;

  tsv.writeRaw("<mantidmatrix>");
  tsv.writeLine("WorkspaceName") << m_strName;
  tsv.writeRaw(app->windowGeometryInfo(this));
  tsv.writeLine("SelectedTab") << m_tabs->currentIndex();
  tsv.writeRaw("</mantidmatrix>");

  return tsv.outputLines();
}

std::vector<std::string> MantidMatrix::getWorkspaceNames() {
  return {m_strName};
}

/**
 * Creates a MantidMatrixTabExtension of a specified type
 * @param type: the type of the tab extension
 */
void MantidMatrix::addMantidMatrixTabExtension(MantidMatrixModel::Type type) {
  // We only want to have unique tab extensions
  if (m_extensions.count(type) > 0) {
    g_log.warning(
        "Tried to add an extension for a type which already has an extension");
    return;
  }

  // Have the extension handler create a new extension and initialize it.
  setupNewExtension(type);
}

/**
 * Hook up the MantidMatrixExtension to the new tab etc
 */
void MantidMatrix::setupNewExtension(MantidMatrixModel::Type type) {
  switch (type) {
  case MantidMatrixModel::DX: {
    // Provide an extension
    auto extension = m_extensionRequest.createMantidMatrixTabExtension(type);

    // We need to hook up the extension
    extension.model = new MantidMatrixModel(this, m_workspace.get(), m_rows,
                                            m_cols, m_startRow, type);
    extension.tableView = std::make_unique<QTableView>();

    // Add a new tab
    m_tabs->insertTab(modelTypeToInt(type), extension.tableView.get(),
                      extension.label);

    // Install the eventfilter
    extension.tableView->installEventFilter(this);

    // Connect Table View
    connectTableView(extension.tableView.get(), extension.model);

    m_extensions.emplace(type, std::move(extension));

    // Set the column width
    auto columnWidth = m_extensionRequest.getColumnWidthPreference(
        type, m_extensions, MantidPreferences::MantidMatrixColumnWidthDx());
    setColumnsWidth(modelTypeToInt(type), columnWidth);

    // Set the number format
    auto format = m_extensionRequest.getFormat(
        type, m_extensions, MantidPreferences::MantidMatrixNumberFormatDx());
    auto precision = m_extensionRequest.getPrecision(
        type, m_extensions, MantidPreferences::MantidMatrixNumberPrecisionDx());
    setNumberFormat(modelTypeToInt(type), format, precision);
    break;
  }
  default:
    throw std::runtime_error("Unknown MantidMatrix extension.");
  }
}

/**
 * Update the existing extensions
 * @param ws: the new workspace
 */
void MantidMatrix::updateExtensions(Mantid::API::MatrixWorkspace_sptr ws) {
  auto it = m_extensions.begin();
  while (it != m_extensions.cend()) {
    auto type = it->first;
    switch (type) {
    case MantidMatrixModel::DX:
      if (ws->hasDx(0)) {
        auto &extension = it->second;
        extension.model = new MantidMatrixModel(this, ws.get(), m_rows, m_cols,
                                                m_startRow, type);
        connectTableView(extension.tableView.get(), extension.model);
        auto format = m_extensionRequest.getFormat(
            type, m_extensions,
            MantidPreferences::MantidMatrixNumberFormatDx());
        auto precision = m_extensionRequest.getPrecision(
            type, m_extensions,
            MantidPreferences::MantidMatrixNumberPrecisionDx());
        setNumberFormat(modelTypeToInt(type), format, precision);
        ++it;
      } else {
        closeDependants();
        m_tabs->removeTab(modelTypeToInt(type));
        it = m_extensions.erase(it);
      }
      break;
    default:
      throw std::runtime_error("Unknown MantidMatrix extension.");
    }
  }
}
