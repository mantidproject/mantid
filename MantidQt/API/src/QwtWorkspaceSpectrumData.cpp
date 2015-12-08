#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/PlotAxis.h"

#include <QStringBuilder>

/**
 * Construct a QwtWorkspaceSpectrumData object with a source workspace
 * @param workspace The workspace containing the data
 * @param specIndex Index of the spectrum to plot
 * @param logScale If true, plot a log scale
 * @param plotAsDistribution If true and the data is histogram and not already a
 * distribution then plot the Y values/X bin-width
 */
QwtWorkspaceSpectrumData::QwtWorkspaceSpectrumData(
    const Mantid::API::MatrixWorkspace &workspace, int specIndex,
    const bool logScale, const bool plotAsDistribution)
    : m_spec(specIndex), m_X(workspace.readX(specIndex)),
      m_Y(workspace.readY(specIndex)), m_E(workspace.readE(specIndex)),
      m_xTitle(), m_yTitle(), m_isHistogram(workspace.isHistogramData()),
      m_dataIsNormalized(workspace.isDistribution()), m_binCentres(false),
      m_logScale(logScale), m_isDistribution(false), m_isWaterfall(false),
      m_offsetX(0), m_offsetY(0) {
  // Actual plotting based on what type of data we have
  setAsDistribution(plotAsDistribution &&
                    !m_dataIsNormalized); // takes into account if this is a
                                          // histogram and sets m_isDistribution

  m_xTitle = MantidQt::API::PlotAxis(workspace, 0).title();
  m_yTitle = MantidQt::API::PlotAxis((m_dataIsNormalized || m_isDistribution),
                                     workspace)
                 .title();

  // Calculate the min and max values
  calculateYMinAndMax(m_Y, m_minY, m_maxY, m_minPositive);
}

/// Virtual copy constructor
QwtWorkspaceSpectrumData *QwtWorkspaceSpectrumData::copy() const {
  return new QwtWorkspaceSpectrumData(*this);
}

/**
 * @param workspace A new workspace source
 * @return
*/
QwtWorkspaceSpectrumData *QwtWorkspaceSpectrumData::copyWithNewSource(
    const Mantid::API::MatrixWorkspace &workspace) const {
  return new QwtWorkspaceSpectrumData(workspace, m_spec, m_logScale,
                                      m_isDistribution);
}

/** Size of the data set
 */
size_t QwtWorkspaceSpectrumData::size() const {
  if (m_binCentres || !m_isHistogram) {
    return m_Y.size();
  }

  return m_X.size();
}

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double QwtWorkspaceSpectrumData::x(size_t i) const {
  double val = m_binCentres ? (m_X[i] + m_X[i + 1]) / 2 : m_X[i];
  return m_isWaterfall ? val + m_offsetX : val;
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double QwtWorkspaceSpectrumData::y(size_t i) const {
  double tmp = i < m_Y.size() ? m_Y[i] : m_Y[m_Y.size() - 1];
  if (m_isDistribution) {
    tmp /= (m_X[i + 1] - m_X[i]);
  }
  if (m_logScale && tmp <= 0.) {
    tmp = m_minPositive;
  }

  return m_isWaterfall ? tmp + m_offsetY : tmp;
}

double QwtWorkspaceSpectrumData::ex(size_t i) const {
  double err = m_isHistogram ? (m_X[i] + m_X[i + 1]) / 2 : m_X[i];
  return m_isWaterfall ? err + m_offsetX : err;
}

double QwtWorkspaceSpectrumData::e(size_t i) const {
  double ei = (i < m_E.size()) ? m_E[i] : m_E[m_E.size() - 1];
  if (m_isDistribution) {
    ei /= (m_X[i + 1] - m_X[i]);
  }
  if (m_logScale) {
    double yi = (i < m_Y.size()) ? m_Y[i] : m_Y[m_Y.size() - 1];
    if (yi <= 0.0)
      return 0;
    else
      return ei;
  } else
    return ei;
}

size_t QwtWorkspaceSpectrumData::esize() const { return m_E.size(); }

/**
 * Depending upon whether the log options have been set.
 * @return the lowest y value.
 */
double QwtWorkspaceSpectrumData::getYMin() const {
  if (m_logScale)
    return m_isWaterfall ? m_minPositive + m_offsetY : m_minPositive;
  else
    return m_isWaterfall ? m_minY + m_offsetY : m_minY;
}

/**
 * Depending upon whether the log options have been set.
 * @return the highest y value.
 */
double QwtWorkspaceSpectrumData::getYMax() const {
  if (m_logScale && m_maxY <= 0)
    return m_isWaterfall ? m_minPositive + m_offsetY : m_minPositive;
  else
    return m_isWaterfall ? m_maxY + m_offsetY : m_maxY;
}

/**
 * @return A string containin the text to use as an X axis label
 */
QString QwtWorkspaceSpectrumData::getXAxisLabel() const { return m_xTitle; }

/**
 * @return A string containin the text to use as an Y axis label
 */
QString QwtWorkspaceSpectrumData::getYAxisLabel() const { return m_yTitle; }

void QwtWorkspaceSpectrumData::setLogScale(bool on) { m_logScale = on; }

void QwtWorkspaceSpectrumData::saveLowestPositiveValue(const double v) {
  if (v > 0)
    m_minPositive = v;
}

bool QwtWorkspaceSpectrumData::setAsDistribution(bool on) {
  m_isDistribution = on && m_isHistogram;
  return m_isDistribution;
}

void QwtWorkspaceSpectrumData::setXOffset(const double x) { m_offsetX = x; }

void QwtWorkspaceSpectrumData::setYOffset(const double y) { m_offsetY = y; }

void QwtWorkspaceSpectrumData::setWaterfallPlot(bool on) { m_isWaterfall = on; }

//-----------------------------------------------------------------------------
// Protected methods
//-----------------------------------------------------------------------------
/**
 * @param rhs A source object whose state is copied here
 * @return A reference to this object
 */
QwtWorkspaceSpectrumData &QwtWorkspaceSpectrumData::
operator=(const QwtWorkspaceSpectrumData &rhs) {
  if (this != &rhs) {
    m_spec = rhs.m_spec;
    m_X = rhs.m_X;
    m_Y = rhs.m_Y;
    m_E = rhs.m_E;
    m_xTitle = rhs.m_xTitle;
    m_yTitle = rhs.m_yTitle;
    m_isHistogram = rhs.m_isHistogram;
    m_binCentres = rhs.m_binCentres;
    m_logScale = rhs.m_logScale;
    m_minPositive = rhs.m_minPositive;
    m_isDistribution = rhs.m_isDistribution;
    m_offsetX = rhs.m_offsetX;
    m_offsetY = rhs.m_offsetY;
    m_isWaterfall = rhs.m_isWaterfall;
  }
  return *this;
}