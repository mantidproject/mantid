// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMatrixFunction.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"

MantidMatrixFunction::MantidMatrixFunction(MantidMatrix &matrix)
    : m_outside(0) {

  init(matrix.workspace());

  double tmp;
  matrix.range(&tmp, &m_outside);
  m_outside *= 1.1;

  m_observer = new MantidMatrixFunctionWorkspaceObserver(this);
  m_observer->observeADSClear();
  m_observer->observePreDelete();
  m_observer->observeAfterReplace();
}

MantidMatrixFunction::~MantidMatrixFunction() {
  m_observer->observeADSClear(false);
  m_observer->observePreDelete(false);
  m_observer->observeAfterReplace(false);
}

/**
 * Initialize the function from a matrix workspace.
 *
 * @param workspace :: New workspace to use.
 */
void MantidMatrixFunction::init(
    const Mantid::API::MatrixWorkspace_const_sptr &workspace) {
  m_workspace = workspace;

  if (!m_workspace->getAxis(1)) {
    throw std::runtime_error("The y-axis is not set");
  }

  setMesh(m_workspace->blocksize(), m_workspace->getNumberHistograms());
}

/**
 * Reset the underlying matrix workspace and the mesh dimensions.
 *
 * @param workspace :: New workspace to use.
 */
void MantidMatrixFunction::reset(
    const Mantid::API::MatrixWorkspace_const_sptr &workspace) {
  init(workspace);
  double minz, maxz;
  findYRange(workspace, minz, maxz);
  setMinZ(minz);
  setMaxZ(maxz);
  setMesh(workspace->blocksize(), workspace->getNumberHistograms());
  double minx, maxx;
  workspace->getXMinMax(minx, maxx);
  const Mantid::API::Axis *axis = workspace->getAxis(1);
  double miny = axis->getMin();
  double maxy = axis->getMax();
  setDomain(minx, maxx, miny, maxy);
  create();
}

double MantidMatrixFunction::operator()(double x, double y) {
  size_t i = indexY(y);
  if (i >= rows()) {
    return m_outside;
  }

  size_t j = indexX(i, x);

  if (j < columns()) {
    return m_workspace->y(i)[j];
  } else {
    return m_outside;
  }
}

double MantidMatrixFunction::getMinPositiveValue() const {
  double zmin = DBL_MAX;
  for (size_t i = 0; i < rows(); ++i) {
    for (size_t j = 0; j < columns(); ++j) {
      double tmp = value(i, j);
      if (tmp > 0 && tmp < zmin) {
        zmin = tmp;
      }
    }
  }
  return zmin;
}

QString MantidMatrixFunction::saveToString() const {
  return "mantidMatrix3D\t";
}

/**
 * Connect to a viewer object to ask it to redraw when needed.
 *
 * @param viewer :: An object displaying this function. It must have slot
 *update().
 */
void MantidMatrixFunction::connectToViewer(QObject *viewer) {
  m_observer->connect(m_observer, SIGNAL(requestRedraw()), viewer,
                      SLOT(update()));
  m_observer->connect(m_observer, SIGNAL(requestClose()), viewer,
                      SLOT(close()));
}

double MantidMatrixFunction::value(size_t row, size_t col) const {
  return m_workspace->y(row)[col];
}

void MantidMatrixFunction::getRowYRange(size_t row, double &ymin,
                                        double &ymax) const {
  const Mantid::API::Axis &yAxis = *(m_workspace->getAxis(1));

  size_t i = row;
  double y = yAxis(i);

  size_t imax = static_cast<int>(m_workspace->getNumberHistograms()) - 1;
  if (yAxis.isNumeric()) {
    if (i < imax) {
      ymax = (yAxis(i + 1) + y) / 2;
      if (i > 0) {
        ymin = (yAxis(i - 1) + y) / 2;
      } else {
        ymin = 2 * y - ymax;
      }
    } else {
      ymin = (yAxis(i - 1) + y) / 2;
      ymax = 2 * y - ymin;
    }
  } else // if spectra
  {
    ymin = y - 0.5;
    ymax = y + 0.5;
  }
}

void MantidMatrixFunction::getRowXRange(int row, double &xmin,
                                        double &xmax) const {
  const auto &X = m_workspace->x(row);
  xmin = X[0];
  xmax = X[X.size() - 1];
}

const Mantid::HistogramData::HistogramX &
MantidMatrixFunction::getHistogramX(int row) const {
  return m_workspace->x(row);
}

/**
 * Performs a binary search for an x value in the x data of a particular
 * spectrum. There are two scenarios to consider which are illustrated by
 * examples
 *
 * 1. Histogram Data:
 * The x value of the example is 6500
 *
 * Y:       6      6       16        6         6
 * X: 2000    4000    8000    12000     16000     20000
 *
 * The algorithm will determine that the index of X which is closest to 6500 is
 *2,
 * but the Y index with the correct data is 1 (since the value should be 6 not
 *16)
 *
 * 2. Point Data:
 * Y:   6      6       16        6         6
 * X: 2000    4000    8000    12000     16000
 *
 * The algorithm will determine that the index of X which is closest to 6500 is
 *2,
 * and the Y index with the correct data is 2 as well since there is a
 *one-to-one
 * mapping between the indices of Y and X.
 *
 * @param row: the workspace index to search in
 * @param xValue: the value to search for
 * @return the index of the Y data which is associated with the x value.
 */
size_t MantidMatrixFunction::indexX(size_t row, double xValue) const {
  auto isHistogram = m_workspace->isHistogramData();
  const auto &X = m_workspace->x(row);
  const auto n = X.size();

  auto provideIndexForPointData = [&X](size_t start, size_t stop, double xValue,
                                       double midValue) {
    if (fabs(X[stop] - xValue) < fabs(midValue - xValue))
      return stop;
    return start;
  };

  if (n == 0 || xValue < X[0] || xValue > X[n - 1]) {
    return std::numeric_limits<size_t>::max();
  }

  size_t start = 0, stop = n - 1, mid = n / 2;
  for (size_t it = 0; it < n; it++) {
    const double midValue = X[mid];
    if (midValue == xValue)
      return mid;

    // If we reach two neighbouring x values, then we need to decide
    // which index to pick.
    if (abs(static_cast<int>(start) - static_cast<int>(stop)) < 2) {
      if (isHistogram) {
        return start;
      } else {
        return provideIndexForPointData(start, stop, xValue, midValue);
      }
    }

    // Reset the interval to search
    if (xValue > midValue)
      start = mid;
    else
      stop = mid;
    mid = start + (stop - start) / 2;
  }

  return start;
}

size_t MantidMatrixFunction::indexY(double s) const {
  size_t n = rows();

  const Mantid::API::Axis &yAxis = *m_workspace->getAxis(1);

  bool isNumeric = yAxis.isNumeric();

  if (n == 0)
    return std::numeric_limits<size_t>::max();

  size_t i0 = 0;

  if (s < yAxis(i0)) {
    if (isNumeric || yAxis(i0) - s > 0.5)
      return std::numeric_limits<size_t>::max();
    return 0;
  } else if (s > yAxis(n - 1)) {
    if (isNumeric || s - yAxis(n - 1) > 0.5)
      return std::numeric_limits<size_t>::max();
    return n - 1;
  }

  size_t i = i0, j = n - 1, k = n / 2;
  for (size_t it = 0; it < n; it++) {
    const double ss = yAxis(k);
    if (ss == s)
      return k;
    if (abs(static_cast<int>(i) - static_cast<int>(j)) < 2) {
      double ds = fabs(ss - s);
      double ds1 = fabs(yAxis(j) - s);
      if (ds1 < ds) {
        if (isNumeric || ds1 < 0.5)
          return j;
        return std::numeric_limits<size_t>::max();
      }
      if (isNumeric || ds < 0.5)
        return i;
      return std::numeric_limits<size_t>::max();
    }
    if (s > ss)
      i = k;
    else
      j = k;
    k = i + (j - i) / 2;
  }

  return i;
}

/*--------------------------------------------------------------------------------------------*/

MantidMatrixFunctionWorkspaceObserver::MantidMatrixFunctionWorkspaceObserver(
    MantidMatrixFunction *fun)
    : m_function(fun) {}

void MantidMatrixFunctionWorkspaceObserver::afterReplaceHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  if (m_function->m_workspace && wsName == m_function->m_workspace->getName()) {
    auto mws =
        boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(ws);
    if (mws) {
      m_function->reset(mws);
      emit requestRedraw();
    } else {
      emit requestClose();
    }
  }
}

void MantidMatrixFunctionWorkspaceObserver::preDeleteHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> /*ws*/) {
  if (m_function->m_workspace && wsName == m_function->m_workspace->getName()) {
    emit requestClose();
  }
}

void MantidMatrixFunctionWorkspaceObserver::clearADSHandle() {
  emit requestClose();
}
