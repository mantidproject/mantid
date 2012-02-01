#include "MantidQtAPI/MantidQwtIMDWorkspaceData.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

/** Constructor
 *
 * @param workspace :: IMDWorkspace to plot
 * @param logScale :: true to plot Y in log scale
 * @param start :: start point in N-dimensions of the line
 * @param end :: end point in N-dimensions of the line
 * @param normalize :: method for normalizing the line
 * @param isDistribution :: is this a distribution (divide by bin width?)
 * @return
 */
MantidQwtIMDWorkspaceData::MantidQwtIMDWorkspaceData(Mantid::API::IMDWorkspace_const_sptr workspace, const bool logScale,
    Mantid::Kernel::VMD start, Mantid::Kernel::VMD end,
    Mantid::API::MDNormalization normalize,
    bool isDistribution)
  : QObject(),
  m_workspace(workspace),
  m_logScale(logScale),
  m_minPositive(0),
  m_start(start),
  m_end(end),
  m_normalization(normalize),
  m_isDistribution(isDistribution),
  m_transform(NULL)
{
  if (start.getNumDims() == 1 && end.getNumDims() == 1)
  {
    if (start[0] == 0.0 && end[0] == 0.0)
    {
      // Default start and end. Find the limits
      std::string alongDim = m_workspace->getNonIntegratedDimensions()[0]->getName();
      size_t nd = m_workspace->getNumDims();
      m_start = VMD(nd);
      m_end = VMD(nd);
      for (size_t d=0; d<nd; d++)
      {
        IMDDimension_const_sptr dim = m_workspace->getDimension(d);
        if (dim->getDimensionId() == alongDim)
        {
          // All the way through in the single dimension
          m_start[d] = dim->getMinimum();
          m_end[d] = dim->getMaximum();
        }
        else
        {
          // Mid point along each dimension
          m_start[d] = (dim->getMaximum() + dim->getMinimum()) / 2;
          m_end[d] = m_start[d];
        }
      }
    }
  }
  // Unit direction of the line
  m_dir = m_end - m_start;
  m_dir.normalize();
  // And cache the X/Y values
  this->cacheLinePlot();
}

//-----------------------------------------------------------------------------
/// Copy constructor
MantidQwtIMDWorkspaceData::MantidQwtIMDWorkspaceData(const MantidQwtIMDWorkspaceData& data)
  : QObject(),
  m_workspace(data.m_workspace),
  m_logScale(data.m_logScale),
  m_start(data.m_start),
  m_end(data.m_end),
  m_dir(data.m_dir),
  m_normalization(data.m_normalization),
  m_isDistribution(data.m_isDistribution)
{
  this->cacheLinePlot();
  this->setTransform(data.m_transform, data.m_dimensionIndex);
}

/// Destructor
MantidQwtIMDWorkspaceData::~MantidQwtIMDWorkspaceData()
{
  delete m_transform;
}

//-----------------------------------------------------------------------------
/** Cloner/virtual copy constructor
 * @return a copy of this
 */
QwtData * MantidQwtIMDWorkspaceData::copy() const
{
  return new MantidQwtIMDWorkspaceData(*this);
}

/// Return a new data object of the same type but with a new workspace
MantidQwtIMDWorkspaceData* MantidQwtIMDWorkspaceData::copy(Mantid::API::IMDWorkspace_sptr workspace)const
{
  return new MantidQwtIMDWorkspaceData(workspace, m_logScale, m_start, m_end,
      m_normalization, m_isDistribution);
}


//-----------------------------------------------------------------------------
/** Set the coordinate transformation to use to calculate the plot X values.
 * The coordinates in the m_workspace will be converted to another,
 * and then the dimensionIndex^th coordinate will be displayed as the X
 *
 * @param transform :: CoordTransform, NULL for none
 * @param dimensionIndex :: index into the output coordinate space.
 * @throw std::runtime_error if the dimension index does not match the coordinate
 */
void MantidQwtIMDWorkspaceData::setTransform(Mantid::API::CoordTransform * transform, size_t dimensionIndex)
{
  m_dimensionIndex = dimensionIndex;
  if (transform)
  {
    m_transform = transform->clone();
    if (m_dimensionIndex >= m_transform->getOutD())
      throw std::runtime_error("DimensionIndex is larger than the number of output dimensions of the transformation");
  }
  else
    m_transform = NULL;
}



//-----------------------------------------------------------------------------
/** Cache the X/Y line plot data from this workspace and start/end points */
void MantidQwtIMDWorkspaceData::cacheLinePlot()
{
  m_workspace->getLinePlot(m_start, m_end, m_normalization, m_lineX, m_Y, m_E);
  std::cout << "MantidQwtIMDWorkspaceData found " << m_Y.size() << " points\n";
}


//-----------------------------------------------------------------------------
/** Size of the data set
 */
size_t MantidQwtIMDWorkspaceData::size() const
{
  return m_Y.size();
}

/** Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double MantidQwtIMDWorkspaceData::x(size_t i) const
{
  double x = m_lineX[i];
  if (m_transform)
  {
    // Coordinates in the workspace being plotted
    VMD wsCoord = m_start + m_dir * x;
    // Transform to the original workspace's coordinates
    VMD originalCoord = m_transform->applyVMD(wsCoord);
    // And pick only that coordinate
    x = originalCoord[m_dimensionIndex];
  }
  return x;
}

/** Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double MantidQwtIMDWorkspaceData::y(size_t i) const
{
  return m_Y[i];
}

/// Returns the x position of the error bar for the i-th data point (bin)
double MantidQwtIMDWorkspaceData::ex(size_t i) const
{
  return (this->x(i) + this->x(i+1)) / 2.0;
}

/// Returns the error of the i-th data point
double MantidQwtIMDWorkspaceData::e(size_t i) const
{
  return m_E[i];
}

/// Number of error bars to plot
size_t MantidQwtIMDWorkspaceData::esize() const
{
  return m_E.size();
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
