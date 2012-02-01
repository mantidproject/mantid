#include "MantidQtAPI/MantidQwtIMDWorkspaceData.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::API::NullCoordTransform;
using Mantid::API::CoordTransform;
using Mantid::API::IMDWorkspace;

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
  m_transform(NULL),
  m_plotAxis(PlotDistance), m_currentPlotAxis(PlotDistance)
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
          m_start[d] = (dim->getMaximum() + dim->getMinimum()) / 2.0;
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
  m_isDistribution(data.m_isDistribution),
  m_originalWorkspace(data.m_originalWorkspace),
  m_originalXDim(data.m_originalXDim), m_originalYDim(data.m_originalYDim),
  m_transform(data.m_transform->clone() ),
  m_dimensionIndex(data.m_dimensionIndex),
  m_plotAxis(data.m_plotAxis), m_currentPlotAxis(data.m_currentPlotAxis)
{
  this->cacheLinePlot();
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
/** Cache the X/Y line plot data from this workspace and start/end points */
void MantidQwtIMDWorkspaceData::cacheLinePlot()
{
  m_workspace->getLinePlot(m_start, m_end, m_normalization, m_lineX, m_Y, m_E);
//  std::cout << "MantidQwtIMDWorkspaceData found " << m_Y.size() << " points\n";
//  std::cout << "Plotting from " << m_start << " to " << m_end << std::endl;
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

//-----------------------------------------------------------------------------
void MantidQwtIMDWorkspaceData::setPlotAxisChoice(PlotAxisChoice choice)
{
  m_plotAxis = choice;
  this->choosePlotAxis();
}

//-----------------------------------------------------------------------------
/** Which original workspace to use relative to the workspace being plotted.
 *
 * @param index :: -1 for PREVIEW mode = plot this workspace.
 * @param originalXDim :: the index of the X dimension in the slice viewer.
 * @param originalYDim :: the index of the Y dimension in the slice viewer.
 */
void MantidQwtIMDWorkspaceData::setOriginalWorkspaceIndex(int index, size_t originalXDim, size_t originalYDim)
{
  m_originalXDim = originalXDim;
  m_originalYDim = originalYDim;

  if (index < 0)
  {
    // Preview mode. No transformation.
    m_originalWorkspace = m_workspace;
    m_transform = new NullCoordTransform(m_workspace->getNumDims());
  }
  else
  {
    m_originalWorkspace = boost::dynamic_pointer_cast<IMDWorkspace>(m_workspace->getOriginalWorkspace(size_t(index)));
    CoordTransform * temp = m_workspace->getTransformToOriginal(size_t(index));
    if (temp)
      m_transform = temp->clone();
  }
  this->choosePlotAxis();
}



//-----------------------------------------------------------------------------
/** Automatically choose which coordinate to use as the X axis,
 * if we selected it to be automatic
 */
void MantidQwtIMDWorkspaceData::choosePlotAxis()
{
  if (m_plotAxis == MantidQwtIMDWorkspaceData::PlotAuto)
  {
    if (m_transform)
    {
      // Find the X and Y of the start and end points, as would be seen in the SliceViewer
      VMD originalStart = m_transform->applyVMD(m_start);
      VMD originalEnd = m_transform->applyVMD(m_end);
      VMD diff = originalEnd - originalStart;
      double d = fabs(diff[m_originalYDim]) - fabs(diff[m_originalXDim]);
      if (d < 1e-3)
        // More line in X than in Y, or nearly the same
        m_currentPlotAxis = MantidQwtIMDWorkspaceData::PlotX;
      else
        // More Y than X
        m_currentPlotAxis = MantidQwtIMDWorkspaceData::PlotY;
    }
    else
      m_currentPlotAxis = PlotDistance;
  }
  else
  {
    m_currentPlotAxis = m_plotAxis;
  }

  // Point to the right index in the ORIGINAL workspace.
  m_dimensionIndex = 0;
  if (m_currentPlotAxis == MantidQwtIMDWorkspaceData::PlotX)
    m_dimensionIndex = m_originalXDim;
  else if (m_currentPlotAxis == MantidQwtIMDWorkspaceData::PlotY)
    m_dimensionIndex = m_originalYDim;
}

//-----------------------------------------------------------------------------
/// @return the label for the X axis
std::string MantidQwtIMDWorkspaceData::getXAxisLabel() const
{
  IMDDimension_const_sptr dimX = m_originalWorkspace->getDimension(m_originalXDim);
  IMDDimension_const_sptr dimY = m_originalWorkspace->getDimension(m_originalYDim);
  std::string xLabel = "";
  switch (m_currentPlotAxis)
  {
  case MantidQwtIMDWorkspaceData::PlotX:
    xLabel = dimX->getName() + " (" + dimX->getUnits() + ")";
    break;
  case MantidQwtIMDWorkspaceData::PlotY:
    xLabel = dimY->getName() + " (" + dimY->getUnits() + ")";
    break;
  default:
    // Distance, or not set.
    xLabel = "Distance from start";
    if (dimX->getUnits() == dimY->getUnits())
      xLabel += " (" + dimX->getUnits() + ")";
    else
      xLabel += " (undefined units)";
    break;
  }
  return xLabel;
}

//-----------------------------------------------------------------------------
/// @return the label for the Y axis
std::string MantidQwtIMDWorkspaceData::getYAxisLabel() const
{
  switch (m_normalization)
  {
  case Mantid::API::NoNormalization:
    return "Signal";
  case Mantid::API::VolumeNormalization:
    return "Signal normalized by volume";
  case Mantid::API::NumEventsNormalization:
    return "Signal normalized by number of events";
  }
  return "Unknown";
}


