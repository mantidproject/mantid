#include "MantidQtAPI/MantidQwtMatrixWorkspaceData.h"
#include <QStringBuilder> // provides % operator for QString

/// Constructor
MantidQwtMatrixWorkspaceData::MantidQwtMatrixWorkspaceData(Mantid::API::MatrixWorkspace_const_sptr workspace,int specIndex, const bool logScale, bool distr)
 : m_workspace(workspace),
   m_spec(specIndex),
   m_X(workspace->readX(specIndex)),
   m_Y(workspace->readY(specIndex)),
   m_E(workspace->readE(specIndex)),
   m_isHistogram(workspace->isHistogramData()),
   m_binCentres(false),
   m_logScale(logScale),
   m_minPositive(0),
   m_isDistribution(distr)
{}

/// Copy constructor
MantidQwtMatrixWorkspaceData::MantidQwtMatrixWorkspaceData(const MantidQwtMatrixWorkspaceData& data)
{
  this->operator =(data);
}

/** Assignment operator */
MantidQwtMatrixWorkspaceData& MantidQwtMatrixWorkspaceData::operator=(const MantidQwtMatrixWorkspaceData &data)
{
  m_workspace = data.m_workspace;
  m_spec = data.m_spec;
  m_X = data.m_workspace->readX(data.m_spec);
  m_Y = data.m_workspace->readY(data.m_spec);
  m_E = data.m_workspace->readE(data.m_spec);
  m_isHistogram = m_workspace->isHistogramData();
  m_binCentres = data.m_binCentres;
  m_logScale = data.m_logScale;
  m_minPositive = 0;
  m_isDistribution = data.m_isDistribution;
  return *this;
}


/** Size of the data set
 */
size_t MantidQwtMatrixWorkspaceData::size() const
{
  if (m_binCentres || !m_isHistogram)
  {
    return m_Y.size();
  }

  return m_X.size();
}

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double MantidQwtMatrixWorkspaceData::x(size_t i) const
{
  return m_binCentres ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double MantidQwtMatrixWorkspaceData::y(size_t i) const
{
  double tmp = i < m_Y.size() ? m_Y[i] : m_Y[m_Y.size()-1];
  if (m_isDistribution)
  {
    tmp /= (m_X[i+1] - m_X[i]);
  }
  if (m_logScale && tmp <= 0.)
  {
    tmp = m_minPositive;
  }

  return tmp;
}

double MantidQwtMatrixWorkspaceData::ex(size_t i) const
{
  return m_isHistogram ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

double MantidQwtMatrixWorkspaceData::e(size_t i) const
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

size_t MantidQwtMatrixWorkspaceData::esize() const
{
  return m_E.size();
}

/**
 * Depending upon whether the log options have been set.
 * @return the lowest y value.
 */
double MantidQwtMatrixWorkspaceData::getYMin() const
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
double MantidQwtMatrixWorkspaceData::getYMax() const
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
QString MantidQwtMatrixWorkspaceData::getXAxisLabel() const
{
  // Deal with axis names
  Mantid::API::Axis* ax = m_workspace->getAxis(0);
  QString xTitle;
  if (ax->unit() && ax->unit()->unitID() != "Empty" )
  {
    xTitle = QString::fromStdString(ax->unit()->caption());
    if ( !ax->unit()->label().empty() )
    {
      xTitle += " (" + QString::fromStdWString(ax->unit()->utf8Label()) + ")";
    }
  }
  else if (!ax->title().empty())
  {
    xTitle = QString::fromStdString(ax->title());
  }
  else
  {
    xTitle = "X axis";
  }
  return xTitle;
}

/**
 * @return A string containin the text to use as an Y axis label
 */
QString MantidQwtMatrixWorkspaceData::getYAxisLabel() const
{
  QString yTitle = QString::fromStdString(m_workspace->YUnitLabel());
  Mantid::API::Axis* ax = m_workspace->getAxis(0);
  if (m_isDistribution && ax->unit())
  {
    const std::string unitID = ax->unit()->unitID();
    if (unitID != "" || unitID != "Empty")
    {
      yTitle = yTitle % " / " % ax->unit()->label().c_str();
    }
  }
  return yTitle;
}

void MantidQwtMatrixWorkspaceData::setLogScale(bool on)
{
  m_logScale = on;
}

void MantidQwtMatrixWorkspaceData::saveLowestPositiveValue(const double v)
{
  if (v > 0) m_minPositive = v;
}

bool MantidQwtMatrixWorkspaceData::setAsDistribution(bool on)
{
  m_isDistribution = on && m_isHistogram;
  return m_isDistribution;
}
