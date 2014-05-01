#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"
#include "MantidQtAPI/PlotAxis.h"

#include <QStringBuilder>

/// Constructor
QwtWorkspaceSpectrumData::QwtWorkspaceSpectrumData(Mantid::API::MatrixWorkspace_const_sptr workspace,int specIndex, const bool logScale, bool distr)
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
QwtWorkspaceSpectrumData::QwtWorkspaceSpectrumData(const QwtWorkspaceSpectrumData& data)
{
  this->operator =(data);
}

/** Assignment operator */
QwtWorkspaceSpectrumData& QwtWorkspaceSpectrumData::operator=(const QwtWorkspaceSpectrumData &data)
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
size_t QwtWorkspaceSpectrumData::size() const
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
double QwtWorkspaceSpectrumData::x(size_t i) const
{
  return m_binCentres ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double QwtWorkspaceSpectrumData::y(size_t i) const
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

double QwtWorkspaceSpectrumData::ex(size_t i) const
{
  return m_isHistogram ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

double QwtWorkspaceSpectrumData::e(size_t i) const
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

size_t QwtWorkspaceSpectrumData::esize() const
{
  return m_E.size();
}

/**
 * Depending upon whether the log options have been set.
 * @return the lowest y value.
 */
double QwtWorkspaceSpectrumData::getYMin() const
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
double QwtWorkspaceSpectrumData::getYMax() const
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
QString QwtWorkspaceSpectrumData::getXAxisLabel() const
{
  return MantidQt::API::PlotAxis(*m_workspace, 0).title();
}

/**
 * @return A string containin the text to use as an Y axis label
 */
QString QwtWorkspaceSpectrumData::getYAxisLabel() const
{
  return MantidQt::API::PlotAxis(*m_workspace).title();
}

void QwtWorkspaceSpectrumData::setLogScale(bool on)
{
  m_logScale = on;
}

void QwtWorkspaceSpectrumData::saveLowestPositiveValue(const double v)
{
  if (v > 0) m_minPositive = v;
}

bool QwtWorkspaceSpectrumData::setAsDistribution(bool on)
{
  m_isDistribution = on && m_isHistogram;
  return m_isDistribution;
}
