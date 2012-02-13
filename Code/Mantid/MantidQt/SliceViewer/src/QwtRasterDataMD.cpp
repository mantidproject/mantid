#include "MantidQtSliceViewer/QwtRasterDataMD.h"
#include <math.h>
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDWorkspace.h"

namespace MantidQt
{
namespace SliceViewer
{

using namespace Mantid;
using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

//-------------------------------------------------------------------------
/// Constructor
QwtRasterDataMD::QwtRasterDataMD()
: m_slicePoint(NULL), m_fast(true), m_zerosAsNan(true),
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
QwtRasterData* QwtRasterDataMD::copy() const
{
  QwtRasterDataMD* out = new QwtRasterDataMD();
  out->m_ws = this->m_ws;
  out->m_dimX = this->m_dimX;
  out->m_dimY = this->m_dimY;
  out->m_nd = this->m_nd;
  out->m_range = this->m_range;
  out->m_slicePoint = new coord_t[m_nd];
  for (size_t d=0; d<m_nd; d++)
    out->m_slicePoint[d] = this->m_slicePoint[d];
  out->m_ws = this->m_ws;
  out->m_fast = this->m_fast;
  out->m_zerosAsNan = this->m_zerosAsNan;
  out->m_normalization = this->m_normalization;
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
  signal_t value = m_ws->getSignalAtCoord(lookPoint, m_normalization);
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
  if (!m_ws) return QSize();
  // Slow mode? Don't give a raster hint. This will be 1 pixel per point
  if (!m_fast) return QSize();
  // Fast mode: use the bin size to guess at the pixel density
  IMDDimension_const_sptr m_X = m_ws->getDimension(m_dimX);
  IMDDimension_const_sptr m_Y = m_ws->getDimension(m_dimY);
  int w = 2 * int(area.width() / m_X->getBinWidth());
  int h = 2 * int(area.height() /m_Y->getBinWidth());
  if (w<10) w = 10;
  if (h<10) h = 10;
//  std::cout << "rasterHint: " << w << std::endl;
  return QSize(w,h);
}

//------------------------------------------------------------------------------------------------------
/** Sets the workspace being displayed
 *
 * @param ws :: IMDWorkspace to show
 */
void QwtRasterDataMD::setWorkspace(Mantid::API::IMDWorkspace_sptr ws)
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
/** Set the slicing parameters
 *
 * @param dimX :: index of the X dimension
 * @param dimY :: index of the Y dimension
 * @param slicePoint :: vector of slice points
 */
void QwtRasterDataMD::setSliceParams(size_t dimX, size_t dimY, std::vector<Mantid::coord_t> & slicePoint)
{
  if (slicePoint.size() != m_nd)
    throw std::runtime_error("QwtRasterDataMD::setSliceParams(): inconsistent vector/number of dimensions size.");
  m_dimX = dimX;
  m_dimY = dimY;
  delete [] m_slicePoint;
  m_slicePoint = new coord_t[slicePoint.size()];
  for (size_t d=0; d<m_nd; d++)
    m_slicePoint[d] = slicePoint[d];
}

} //namespace
} //namespace
