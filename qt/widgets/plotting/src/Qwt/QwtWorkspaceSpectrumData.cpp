// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/QwtWorkspaceSpectrumData.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/PlotAxis.h"

#include <QStringBuilder>

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
      m_X(workspace.readX(wsIndex)), m_Y(workspace.readY(wsIndex)),
      m_E(workspace.readE(wsIndex)), m_xTitle(), m_yTitle(),
      m_isHistogram(workspace.isHistogramData()),
      m_dataIsNormalized(workspace.isDistribution()), m_binCentres(false),
      m_isDistribution(false) {
  // Actual plotting based on what type of data we have
  setAsDistribution(plotAsDistribution &&
                    !m_dataIsNormalized); // takes into account if this is a
                                          // histogram and sets m_isDistribution

  m_xTitle = MantidQt::API::PlotAxis(workspace, 0).title();
  m_yTitle = MantidQt::API::PlotAxis((m_dataIsNormalized || m_isDistribution),
                                     workspace)
                 .title();

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
size_t QwtWorkspaceSpectrumData::size() const {
  if (!isPlottable()) {
    return 0;
  }
  if (m_binCentres || m_isHistogram) {
    return m_Y.size();
  }

  return m_X.size();
}

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double QwtWorkspaceSpectrumData::getX(size_t i) const {
  return m_binCentres ? (m_X[i] + m_X[i + 1]) / 2 : m_X[i];
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double QwtWorkspaceSpectrumData::getY(size_t i) const {
  double tmp = i < m_Y.size() ? m_Y[i] : m_Y.back();
  if (m_isDistribution) {
    tmp /= fabs(m_X[i + 1] - m_X[i]);
  }
  return tmp;
}

double QwtWorkspaceSpectrumData::getEX(size_t i) const {
  return m_isHistogram ? (m_X[i] + m_X[i + 1]) / 2 : m_X[i];
}

double QwtWorkspaceSpectrumData::getE(size_t i) const {
  double ei = (i < m_E.size()) ? m_E[i] : m_E.back();
  if (m_isDistribution) {
    ei /= fabs(m_X[i + 1] - m_X[i]);
  }
  return ei;
}

size_t QwtWorkspaceSpectrumData::esize() const {
  if (!isPlottable()) {
    return 0;
  }
  return m_E.size();
}

/**
 * @return A string containin the text to use as an X axis label
 */
QString QwtWorkspaceSpectrumData::getXAxisLabel() const { return m_xTitle; }

/**
 * @return A string containin the text to use as an Y axis label
 */
QString QwtWorkspaceSpectrumData::getYAxisLabel() const { return m_yTitle; }

bool QwtWorkspaceSpectrumData::setAsDistribution(bool on) {
  m_isDistribution = on && m_isHistogram;
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
    m_wsIndex = rhs.m_wsIndex;
    m_X = rhs.m_X;
    m_Y = rhs.m_Y;
    m_E = rhs.m_E;
    m_xTitle = rhs.m_xTitle;
    m_yTitle = rhs.m_yTitle;
    m_isHistogram = rhs.m_isHistogram;
    m_binCentres = rhs.m_binCentres;
  }
  return *this;
}
