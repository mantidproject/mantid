#include "MantidQtAPI/QwtRasterDataMD.h"
#include <math.h>
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt
{
namespace API
{

using namespace Mantid;
using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

//-------------------------------------------------------------------------
/// Constructor
QwtRasterDataMD::QwtRasterDataMD()
: m_ws(), m_overlayWS(),
  m_slicePoint(NULL),
  m_overlayXMin(0.0), m_overlayXMax(0.0),
  m_overlayYMin(0.0), m_overlayYMax(0.0),
  m_overlayInSlice(false),
  m_fast(true), m_zerosAsNan(true),
  m_normalization(Mantid::API::VolumeNormalization)
{
  m_range = QwtDoubleInterval(0.0, 1.0);
  m_nd = 0;
  m_dimX = 0;
  m_dimY = 0;
  nan = std::numeric_limits<double>::quiet_NaN();
}

//-------------------------------------------------------------------------
/// Destructor
QwtRasterDataMD::~QwtRasterDataMD()
{
  delete [] m_slicePoint;
}


//-------------------------------------------------------------------------
/** Perform a copy of this data object */
QwtRasterDataMD *QwtRasterDataMD::copy() const
{
  QwtRasterDataMD* out = new QwtRasterDataMD();
  this->copyFrom(*this, *out);
  return out;
}

//-------------------------------------------------------------------------
/** Set the data range (min/max) to display */
void QwtRasterDataMD::setRange(const QwtDoubleInterval & range)
{ m_range = range; }



//-------------------------------------------------------------------------
/** Return the data value to plot at the given position
 *
 * @param x :: position in coordinates of the MDWorkspace
 * @param y :: position in coordinates of the MDWorkspace
 * @return signal to plot
 */
double QwtRasterDataMD::value(double x, double y) const
{
  if (!m_ws) return 0;

  // Generate the vector of coordinates, filling in X and Y
  coord_t * lookPoint = new coord_t[m_nd];
  for (size_t d=0; d<m_nd; d++)
  {
    if (d==m_dimX)
      lookPoint[d] = static_cast<coord_t>(x);
    else if (d==m_dimY)
      lookPoint[d] = static_cast<coord_t>(y);
    else
      lookPoint[d] = m_slicePoint[d];
  }

  // Get the signal at that point
  signal_t value = 0;

  // Check if the overlay WS is within range of being viewed
  if (m_overlayWS && m_overlayInSlice
      && (x >= m_overlayXMin) && (x < m_overlayXMax)
      && (y >= m_overlayYMin) && (y < m_overlayYMax))
  {
    // Point is in the overlaid workspace
    value = m_overlayWS->getSignalAtCoord(lookPoint, m_normalization);
  }
  else
  {
    // No overlay, or not within range of that workspace
    value = m_ws->getSignalAtCoord(lookPoint, m_normalization);
  }
  delete [] lookPoint;

  // Special case for 0 = show as NAN
  if (m_zerosAsNan && value == 0.)
    return nan;

  return value;
}


//------------------------------------------------------------------------------------------------------
/** Return the data range to show */
QwtDoubleInterval QwtRasterDataMD::range() const
{
  // Linear color plot
  return m_range;
}

//------------------------------------------------------------------------------------------------------
/** Set to use "fast" rendering mode
 * @param fast :: if true, will guess at the number of pixels to render based
 * on workspace resolution
 */
void QwtRasterDataMD::setFastMode(bool fast)
{
  this->m_fast = fast;
}

//------------------------------------------------------------------------------------------------------
/** Set to convert Zeros to NAN to make them transparent when displaying
 *
 * @param val :: true to make 0 = nan
 */
void QwtRasterDataMD::setZerosAsNan(bool val)
{
  this->m_zerosAsNan = val;
}

//------------------------------------------------------------------------------------------------------
/** Set how the signal is normalized
 *
 * @param normalization :: option from MDNormalization enum.
 */
void QwtRasterDataMD::setNormalization(Mantid::API::MDNormalization normalization)
{
  m_normalization = normalization;
}

/** @return how the signal is normalized */
Mantid::API::MDNormalization QwtRasterDataMD::getNormalization() const
{
  return m_normalization;
}

//------------------------------------------------------------------------------------------------------
/** Return how many pixels this area should be rendered as
 *
 * @param area :: area under view
 * @return # of pixels in each direction
 */
QSize QwtRasterDataMD::rasterHint(const QwtDoubleRect &area) const
{
  if (!m_ws || !m_X || !m_Y) return QSize();
  // Slow mode? Don't give a raster hint. This will be 1 pixel per point
  if (!m_fast) return QSize();

  // Fast mode: use the bin size to guess at the pixel density
  coord_t binX = m_X->getBinWidth();
  coord_t binY = m_Y->getBinWidth();

  // Use the overlay workspace, if any, and if its bins are smaller
  if (m_overlayWS && m_overlayInSlice)
  {
    coord_t temp;
    temp = m_overlayWS->getDimension(m_dimX)->getBinWidth();
    if (temp < binX) binX = temp;
    temp = m_overlayWS->getDimension(m_dimY)->getBinWidth();
    if (temp < binY) binY = temp;
  }

  int w = 3 * int(area.width() / binX);
  int h = 3 * int(area.height() / binY);
  if (w<10) w = 10;
  if (h<10) h = 10;
  return QSize(w,h);
}

//------------------------------------------------------------------------------------------------------
/** Sets the workspace being displayed
 *
 * @param ws :: IMDWorkspace to show
 */
void QwtRasterDataMD::setWorkspace(IMDWorkspace_const_sptr ws)
{
  if (!ws)
    throw std::runtime_error("QwtRasterDataMD::setWorkspace(): NULL workspace passed.");
  m_ws = ws;
  m_nd = m_ws->getNumDims();
  m_dimX = 0;
  m_dimY = 1;
  delete [] m_slicePoint;
  m_slicePoint = new coord_t[m_nd];
}

//------------------------------------------------------------------------------------------------------
/** Gets the workspace being displayed
 */
Mantid::API::IMDWorkspace_const_sptr QwtRasterDataMD::getWorkspace() const
{
  return m_ws;
}

//------------------------------------------------------------------------------------------------------
/** Sets the workspace that will be displayed ON TOP of the original workspace.
 * For dynamic rebinning.
 *
 * @param ws :: IMDWorkspace to show
 */
void QwtRasterDataMD::setOverlayWorkspace(Mantid::API::IMDWorkspace_const_sptr ws)
{
  if (!ws)
  {
    m_overlayWS.reset();
    return;
  }
  if (ws->getNumDims() != m_nd)
    throw std::runtime_error("QwtRasterDataMD::setOverlayWorkspace(): workspace does not have the same number of dimensions!");
  m_overlayWS = ws;
}

//------------------------------------------------------------------------------------------------------
/** Set the slicing parameters
 *
 * @param dimX :: index of the X dimension
 * @param dimY :: index of the Y dimension
 * @param X : X Dimension
 * @param Y : Y Dimension
 * @param slicePoint :: vector of slice points
 */
void QwtRasterDataMD::setSliceParams(size_t dimX, size_t dimY,
    Mantid::Geometry::IMDDimension_const_sptr X, Mantid::Geometry::IMDDimension_const_sptr Y,
    std::vector<Mantid::coord_t> & slicePoint)
{
  if (slicePoint.size() != m_nd)
    throw std::runtime_error("QwtRasterDataMD::setSliceParams(): inconsistent vector/number of dimensions size.");
  m_dimX = dimX;
  m_dimY = dimY;
  m_X = X;
  m_Y = Y;
  if (!m_X || !m_Y)
    throw std::runtime_error("QwtRasterDataMD::setSliceParams(): one of the input dimensions is NULL");
  delete [] m_slicePoint;
  m_slicePoint = new coord_t[slicePoint.size()];
  m_overlayInSlice = true;
  for (size_t d=0; d<m_nd; d++)
  {
    m_slicePoint[d] = slicePoint[d];
    // Don't show the overlay WS if it is outside of range in the slice points
    if (m_overlayWS && d != m_dimX && d != m_dimY)
    {
      if (slicePoint[d] < m_overlayWS->getDimension(d)->getMinimum()
          || slicePoint[d] >= m_overlayWS->getDimension(d)->getMaximum())
        m_overlayInSlice = false;
    }
  }
  // Cache the edges of the overlaid workspace
  if (m_overlayWS)
  {
    m_overlayXMin = m_overlayWS->getDimension(m_dimX)->getMinimum();
    m_overlayXMax = m_overlayWS->getDimension(m_dimX)->getMaximum();
    m_overlayYMin = m_overlayWS->getDimension(m_dimY)->getMinimum();
    m_overlayYMax = m_overlayWS->getDimension(m_dimY)->getMaximum();
  }
}

//-----------------------------------------------------------------------------
// Protected members
//-----------------------------------------------------------------------------

/**
 * Copy settings from one object to another
 * @param source A source object to copy from
 * @param dest The destination object that receives the contents
 */
void QwtRasterDataMD::copyFrom(const QwtRasterDataMD &source, QwtRasterDataMD &dest) const
{
  //base bounding box
  dest.setBoundingRect(source.boundingRect());

  dest.m_ws = source.m_ws;
  dest.m_dimX = source.m_dimX;
  dest.m_dimY = source.m_dimY;
  dest.m_nd = source.m_nd;
  dest.m_range = source.m_range;
  dest.m_slicePoint = new coord_t[m_nd];
  for (size_t d=0; d<m_nd; d++)
    dest.m_slicePoint[d] = source.m_slicePoint[d];
  dest.m_ws = source.m_ws;
  dest.m_fast = source.m_fast;
  dest.m_zerosAsNan = source.m_zerosAsNan;
  dest.m_normalization = source.m_normalization;

  dest.m_overlayWS = source.m_overlayWS;
  dest.m_overlayXMin = source.m_overlayXMin;
  dest.m_overlayXMax = source.m_overlayXMax;
  dest.m_overlayYMin = source.m_overlayYMin;
  dest.m_overlayYMax = source.m_overlayYMax;
  dest.m_overlayInSlice = source.m_overlayInSlice;
}

} //namespace
} //namespace
