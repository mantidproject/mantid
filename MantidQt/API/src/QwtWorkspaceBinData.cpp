#include "MantidQtAPI/QwtWorkspaceBinData.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/PlotAxis.h"

#include <QStringBuilder>

/// Constructor
QwtWorkspaceBinData::QwtWorkspaceBinData(
    const Mantid::API::MatrixWorkspace &workspace, int binIndex,
    const bool logScale)
    : m_binIndex(binIndex), m_X(), m_Y(), m_E(), m_xTitle(), m_yTitle(),
      m_logScale(logScale), m_isWaterfall(false), m_offsetX(0), m_offsetY(0) {
  init(workspace);
}

///
QwtWorkspaceBinData *QwtWorkspaceBinData::copy() const {
  return new QwtWorkspaceBinData(*this);
}

/**
 * @param workspace A reference to a different workspace
 * @return A new data object
 */
QwtWorkspaceBinData *QwtWorkspaceBinData::copyWithNewSource(
    const Mantid::API::MatrixWorkspace &workspace) const {
  return new QwtWorkspaceBinData(workspace, m_binIndex, m_logScale);
}

/** Size of the data set
 */
size_t QwtWorkspaceBinData::size() const { return m_Y.size(); }

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double QwtWorkspaceBinData::x(size_t i) const {
  double val = m_isWaterfall ? m_X[i] + m_offsetX : m_X[i];
  return val;
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double QwtWorkspaceBinData::y(size_t i) const {
  Mantid::signal_t val = m_Y[i];
  if (m_logScale && val <= 0.)
    return m_isWaterfall ? m_minPositive + m_offsetY : m_minPositive;
  else
    return m_isWaterfall ? val + m_offsetY : val;
}

double QwtWorkspaceBinData::ex(size_t i) const {
  double err = m_isWaterfall ? m_X[i] + m_offsetX : m_X[i];
  return err;
}

double QwtWorkspaceBinData::e(size_t i) const {
  if (m_logScale) {
    if (m_Y[i] <= 0.0)
      return 0;
    else
      return m_E[i];
  } else
    return m_E[i];
}

size_t QwtWorkspaceBinData::esize() const { return this->size(); }

/**
 * Depending upon whether the log options have been set.
 * @return the lowest y value.
 */
double QwtWorkspaceBinData::getYMin() const {
  if (m_logScale)
    return m_isWaterfall ? m_minPositive + m_offsetY : m_minPositive;
  else
    return m_isWaterfall ? m_minY + m_offsetY : m_minY;
}

/**
 * Depending upon whether the log options have been set.
 * @return the highest y value.
 */
double QwtWorkspaceBinData::getYMax() const {
  if (m_logScale && m_maxY <= 0)
    return m_isWaterfall ? m_minPositive + m_offsetY : m_minPositive;
  else
    return m_isWaterfall ? m_maxY + m_offsetY : m_maxY;
}

/**
 * @return A string containin the text to use as an X axis label
 */
QString QwtWorkspaceBinData::getXAxisLabel() const { return m_xTitle; }

/**
 * @return A string containin the text to use as an Y axis label
 */
QString QwtWorkspaceBinData::getYAxisLabel() const { return m_yTitle; }

void QwtWorkspaceBinData::setLogScale(bool on) { m_logScale = on; }

void QwtWorkspaceBinData::saveLowestPositiveValue(const double v) {
  if (v > 0)
    m_minPositive = v;
}

void QwtWorkspaceBinData::setXOffset(const double x) { m_offsetX = x; }

void QwtWorkspaceBinData::setYOffset(const double y) { m_offsetY = y; }

void QwtWorkspaceBinData::setWaterfallPlot(bool on) { m_isWaterfall = on; }

/**
 * @param rhs A source object whose state is copied here
 * @return A reference to this object
 */
QwtWorkspaceBinData &QwtWorkspaceBinData::
operator=(const QwtWorkspaceBinData &rhs) {
  if (this != &rhs) {
    m_binIndex = rhs.m_binIndex;
    m_X = rhs.m_X;
    m_Y = rhs.m_Y;
    m_E = rhs.m_E;
    m_xTitle = rhs.m_xTitle;
    m_yTitle = rhs.m_yTitle;
    m_logScale = rhs.m_logScale;
    m_minPositive = rhs.m_minPositive;
    m_offsetX = rhs.m_offsetX;
    m_offsetY = rhs.m_offsetY;
    m_isWaterfall = rhs.m_isWaterfall;
  }
  return *this;
}

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------
/**
 * @param workspace A reference to the workspace object that this data refers to
 */
void QwtWorkspaceBinData::init(const Mantid::API::MatrixWorkspace &workspace) {
  if (workspace.axes() != 2) {
    std::ostringstream os;
    os << "QwtWorkspaceBinData(): Workspace must have two axes, found "
       << workspace.axes();
    throw std::invalid_argument(os.str());
  }

  // Check binIndex is valid
  if (static_cast<size_t>(m_binIndex) >= workspace.blocksize()) {
    std::ostringstream os;
    os << "QwtWorkspaceBinData(): Index out of range. index=" << m_binIndex
       << ", nvalues=" << workspace.blocksize();
    throw std::out_of_range(os.str());
  }

  // Fill vectors of data
  const size_t nhist = workspace.getNumberHistograms();
  auto *vertAxis = workspace.getAxis(1); // supplies X values
  m_X.resize(nhist);
  m_Y.resize(nhist);
  m_E.resize(nhist);
  for (size_t i = 0; i < nhist; ++i) {
    m_X[i] = vertAxis->getValue(i);
    m_Y[i] = workspace.readY(i)[m_binIndex];
    m_E[i] = workspace.readE(i)[m_binIndex];
  }

  // meta data
  m_xTitle = MantidQt::API::PlotAxis(workspace, 1).title();
  m_yTitle = MantidQt::API::PlotAxis(false, workspace).title();

  // Calculate the min and max values
  calculateYMinAndMax(m_Y, m_minY, m_maxY, m_minPositive);
}