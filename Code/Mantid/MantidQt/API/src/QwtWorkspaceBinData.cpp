#include "MantidQtAPI/QwtWorkspaceBinData.h"
#include "MantidQtAPI/PlotAxis.h"

#include <QStringBuilder>

/// Constructor
QwtWorkspaceBinData::QwtWorkspaceBinData(const Mantid::API::MatrixWorkspace &workspace, int binIndex, const bool logScale)
 : m_binIndex(binIndex), m_X(), m_Y(), m_E(), m_xTitle(), m_yTitle(),
   m_logScale(logScale),
   m_minPositive(0)
{
  init(workspace);
}

///
QwtWorkspaceBinData *QwtWorkspaceBinData::copy() const
{
  return new QwtWorkspaceBinData(*this);
}

/**
 * @param workspace A reference to a different workspace
 * @return A new data object
 */
QwtWorkspaceBinData *QwtWorkspaceBinData::copyWithNewSource(const Mantid::API::MatrixWorkspace &workspace) const
{
  return new QwtWorkspaceBinData(workspace, m_binIndex, m_logScale);
}


/** Size of the data set
 */
size_t QwtWorkspaceBinData::size() const
{
  return m_Y.size();
}

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double QwtWorkspaceBinData::x(size_t i) const
{
  return m_X[i];
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double QwtWorkspaceBinData::y(size_t i) const
{
  double tmp = m_Y[i];
  if (m_logScale && tmp <= 0.)
  {
    tmp = m_minPositive;
  }
  return tmp;
}

double QwtWorkspaceBinData::ex(size_t i) const
{
  return m_X[i];
}

double QwtWorkspaceBinData::e(size_t i) const
{
  if (m_logScale)
  {
    if (m_Y[i] <= 0.0)
      return 0;
    else
      return m_E[i];
  }
  else
    return m_E[i];
}

size_t QwtWorkspaceBinData::esize() const
{
  return this->size();
}

/**
 * Depending upon whether the log options have been set.
 * @return the lowest y value.
 */
double QwtWorkspaceBinData::getYMin() const
{
  auto it = std::min_element(m_Y.begin(), m_Y.end());
  double temp = 0;
  if(it != m_Y.end())
  {
    temp = *it;
  }
  if (m_logScale && temp <= 0.)
  {
    temp = m_minPositive;
  }
  return temp;
}

/**
 * Depending upon whether the log options have been set.
 * @return the highest y value.
 */
double QwtWorkspaceBinData::getYMax() const
{
  auto it = std::max_element(m_Y.begin(), m_Y.end());
  double temp = 0;
  if(it != m_Y.end())
  {
    temp = *it;
  }
  if (m_logScale && temp <= 0.)
  {
    temp = m_minPositive;
  }
  return temp;
}

/**
 * @return A string containin the text to use as an X axis label
 */
QString QwtWorkspaceBinData::getXAxisLabel() const
{
  return m_xTitle;
}

/**
 * @return A string containin the text to use as an Y axis label
 */
QString QwtWorkspaceBinData::getYAxisLabel() const
{
  return m_yTitle;
}

void QwtWorkspaceBinData::setLogScale(bool on)
{
  m_logScale = on;
}

void QwtWorkspaceBinData::saveLowestPositiveValue(const double v)
{
  if (v > 0) m_minPositive = v;
}

/**
 * @param rhs A source object whose state is copied here
 * @return A reference to this object
 */
QwtWorkspaceBinData &QwtWorkspaceBinData::operator=(const QwtWorkspaceBinData & rhs)
{
  if(this != &rhs)
  {
    m_binIndex = rhs.m_binIndex;
    m_X = rhs.m_X;
    m_Y = rhs.m_Y;
    m_E = rhs.m_E;
    m_xTitle = rhs.m_xTitle;
    m_yTitle = rhs.m_yTitle;
    m_logScale = rhs.m_logScale;
    m_minPositive = rhs.m_minPositive;
  }
  return *this;
}

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------
/**
 * @param workspace A reference to the workspace object that this data refers to
 */
void QwtWorkspaceBinData::init(const Mantid::API::MatrixWorkspace &workspace)
{
  if(workspace.axes() != 2)
  {
    std::ostringstream os;
    os << "QwtWorkspaceBinData(): Workspace must have two axes, found "
       << workspace.axes();
    throw std::invalid_argument(os.str());
  }

  // Check binIndex is valid
  if(static_cast<size_t>(m_binIndex) >= workspace.blocksize())
  {
    std::ostringstream os;
    os << "QwtWorkspaceBinData(): Index out of range. index="
       << m_binIndex << ", nvalues=" << workspace.blocksize();
    throw std::out_of_range(os.str());
  }

  // Fill vectors of data
  const size_t nhist = workspace.getNumberHistograms();
  auto* vertAxis = workspace.getAxis(1); //supplies X values
  m_X.resize(nhist);
  m_Y.resize(nhist);
  m_E.resize(nhist);
  for(size_t i = 0; i < nhist; ++i)
  {
    m_X[i] = vertAxis->getValue(i);
    m_Y[i] = workspace.readY(i)[m_binIndex];
    m_E[i] = workspace.readE(i)[m_binIndex];
  }

  // meta data
  m_xTitle = MantidQt::API::PlotAxis(workspace, 1).title();
  m_yTitle = MantidQt::API::PlotAxis(false, workspace).title();
}
