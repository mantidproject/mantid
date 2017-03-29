#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/PlotAxis.h"

#include <QStringBuilder>

using namespace Mantid::HistogramData;

/**
 * Construct a QwtWorkspaceSpectrumData object with a source workspace
 * @param workspace The workspace containing the data
 * @param wsIndex Index of the spectrum to plot
 * @param logScaleY If true, plot a log scale
 * @param plotAsDistribution If true and the data is histogram and not already a
 * distribution then plot the Y values/X bin-width
 */
QwtWorkspaceSpectrumData::QwtWorkspaceSpectrumData(
    const Mantid::API::MatrixWorkspace &workspace, int wsIndex,
    const bool logScaleY, const bool plotAsDistribution)
    : MantidQwtMatrixWorkspaceData(logScaleY), m_wsIndex(wsIndex),
      m_hist(workspace.binEdges(wsIndex), workspace.counts(wsIndex),
             workspace.countStandardDeviations(wsIndex)),
      m_xTitle(), m_yTitle(), m_dataIsNormalized(workspace.isDistribution()),
      m_binCentres(false), m_isDistribution(false) {

  // Actual plotting based on what type of data we have
  setAsDistribution(plotAsDistribution && !m_dataIsNormalized);

  m_xTitle = MantidQt::API::PlotAxis(workspace, 0).title();
  m_yTitle = MantidQt::API::PlotAxis((m_dataIsNormalized || m_isDistribution),
                                     workspace).title();

  // Calculate the min and max values
  calculateYMinAndMax();
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
  return new QwtWorkspaceSpectrumData(workspace, m_wsIndex, logScaleY(),
                                      m_isDistribution);
}

/** Size of the data set
 */
size_t QwtWorkspaceSpectrumData::size() const { return m_hist.size(); }

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double QwtWorkspaceSpectrumData::getX(size_t i) const {
  return m_binCentres ? m_hist.points()[i] : m_hist.binEdges()[i];
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double QwtWorkspaceSpectrumData::getY(size_t i) const {
  return m_isDistribution ? m_hist.frequencies()[i] : m_hist.counts()[i];
}

double QwtWorkspaceSpectrumData::getEX(size_t i) const { return m_hist.x()[i]; }

double QwtWorkspaceSpectrumData::getE(size_t i) const {
  return m_isDistribution ? m_hist.frequencyStandardDeviations()[i]
                          : m_hist.countStandardDeviations()[i];
}

size_t QwtWorkspaceSpectrumData::esize() const { return m_hist.e().size(); }

/**
 * @return A string containin the text to use as an X axis label
 */
QString QwtWorkspaceSpectrumData::getXAxisLabel() const { return m_xTitle; }

/**
 * @return A string containin the text to use as an Y axis label
 */
QString QwtWorkspaceSpectrumData::getYAxisLabel() const { return m_yTitle; }

bool QwtWorkspaceSpectrumData::setAsDistribution(bool on) {
  m_isDistribution = on;
  return m_isDistribution;
}

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
    static_cast<MantidQwtMatrixWorkspaceData &>(*this) = rhs;
    m_wsIndex = rhs.m_wsIndex;
    m_hist = rhs.m_hist;
    m_xTitle = rhs.m_xTitle;
    m_yTitle = rhs.m_yTitle;
    m_binCentres = rhs.m_binCentres;
  }
  return *this;
}
