// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/QwtWorkspaceBinData.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/PlotAxis.h"

#include <QStringBuilder>
#include <sstream>

/// Constructor
QwtWorkspaceBinData::QwtWorkspaceBinData(
    const Mantid::API::MatrixWorkspace &workspace, int binIndex,
    const bool logScaleY)
    : MantidQwtMatrixWorkspaceData(logScaleY), m_binIndex(binIndex), m_X(),
      m_Y(), m_E(), m_xTitle(), m_yTitle() {
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
  return new QwtWorkspaceBinData(workspace, m_binIndex, logScaleY());
}

/** Size of the data set
 */
size_t QwtWorkspaceBinData::size() const {
  if (!isPlottable()) {
    return 0;
  }
  return m_Y.size();
}

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double QwtWorkspaceBinData::getX(size_t i) const { return m_X[i]; }

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double QwtWorkspaceBinData::getY(size_t i) const { return m_Y[i]; }

double QwtWorkspaceBinData::getEX(size_t i) const { return m_X[i]; }

double QwtWorkspaceBinData::getE(size_t i) const { return m_E[i]; }

// size_t QwtWorkspaceBinData::esize() const { return this->size(); }

/**
 * @return A string containin the text to use as an X axis label
 */
QString QwtWorkspaceBinData::getXAxisLabel() const { return m_xTitle; }

/**
 * @return A string containin the text to use as an Y axis label
 */
QString QwtWorkspaceBinData::getYAxisLabel() const { return m_yTitle; }

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
  calculateYMinAndMax();
}
