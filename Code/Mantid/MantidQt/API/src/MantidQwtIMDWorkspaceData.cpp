#include "MantidQtAPI/MantidQwtIMDWorkspaceData.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

/// Constructor
MantidQwtIMDWorkspaceData::MantidQwtIMDWorkspaceData(Mantid::API::IMDWorkspace_const_sptr workspace,const bool logScale, bool isDistribution)
: QObject(),
m_workspace(workspace),
m_logScale(logScale),
m_minPositive(0),
m_isDistribution(isDistribution)
{}

/// Copy constructor
MantidQwtIMDWorkspaceData::MantidQwtIMDWorkspaceData(const MantidQwtIMDWorkspaceData& data)
: QObject(),
m_workspace(data.m_workspace),
m_logScale(data.m_logScale),
m_minPositive(0),
m_isDistribution(data.m_isDistribution)
{}

/** Size of the data set
 */
size_t MantidQwtIMDWorkspaceData::size() const
{
  return m_workspace->getNonIntegratedDimensions()[0]->getNBins();
}

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double MantidQwtIMDWorkspaceData::x(size_t i) const
{
  Mantid::Geometry::IMDDimension_const_sptr dimension = m_workspace->getNonIntegratedDimensions()[0];
  //Linear mapping from i as index to actual x value.
  double minimum = dimension->getMinimum();
  double maximum = dimension->getMaximum();
  size_t nbins = dimension->getNBins();
  return minimum + double(i)*(maximum - minimum)/(double(nbins));
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double MantidQwtIMDWorkspaceData::y(size_t i) const
{
  Mantid::Geometry::IMDDimension_const_sptr dimension = m_workspace->getNonIntegratedDimensions()[0];
  Mantid::API::IMDIterator* it = m_workspace->createIterator();
  size_t counter = 0;
  Mantid::signal_t signal;
  while(true)
  {
    signal = it->getNormalizedSignal();
    if(counter == i)
    {
      break;
    }
    counter++;
    it->next();
  }
  delete it;
  return signal;
}

double MantidQwtIMDWorkspaceData::ex(size_t i) const
{
  return x(i);
}

double MantidQwtIMDWorkspaceData::e(size_t i) const
{
  Mantid::Geometry::IMDDimension_const_sptr dimension = m_workspace->getNonIntegratedDimensions()[0];
  Mantid::API::IMDIterator* it = m_workspace->createIterator();
  size_t counter = 0;
  Mantid::signal_t error;
  while(true)
  {
    error = it->getNormalizedError();
    if(counter == i)
    {
      break;
    }
    counter++;
    it->next();
  }
  delete it;
  return error;
}

size_t MantidQwtIMDWorkspaceData::esize() const
{
  return m_workspace->getNonIntegratedDimensions()[0]->getNBins();
}

bool MantidQwtIMDWorkspaceData::sameWorkspace(Mantid::API::IMDWorkspace_sptr workspace)const
{
  return workspace.get() == m_workspace.get();
}

void MantidQwtIMDWorkspaceData::setLogScale(bool on)
{
  m_logScale = on;
}

void MantidQwtIMDWorkspaceData::saveLowestPositiveValue(const double v)
{
  if (v > 0) m_minPositive = v;
}

void MantidQwtIMDWorkspaceData::applyOffsets(const double, const double)
{
  std::runtime_error("MantidQwtIMDWorkspaceData::applyOffsets not implemented");
}

bool MantidQwtIMDWorkspaceData::setAsDistribution(bool on)
{
  m_isDistribution = on;
  return m_isDistribution;
}
