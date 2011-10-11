#include "QwtRasterDataMD.h"
#include <math.h>
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid;
using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

QwtRasterDataMD::QwtRasterDataMD()
: m_slicePoint(NULL)
{
  timesRequested = 0;
}


QwtRasterDataMD::~QwtRasterDataMD()
{
  delete [] m_slicePoint;
}


double QwtRasterDataMD::value(double x, double y) const
{
  if (!m_ws) return 0;
//  timesRequested++;
//  if (timesRequested % 1000 == 0)
//    std::cout << timesRequested/1000 << ", ";

  // Generate the vector of coordinates, filling in X and Y
  coord_t * lookPoint = new coord_t[m_nd];
  for (size_t d=0; d<m_nd; d++)
  {
    if (d==m_dimX)
      lookPoint[d] = coord_t(x);
    else if (d==m_dimY)
      lookPoint[d] = coord_t(y);
    else
      lookPoint[d] = m_slicePoint[d];
  }
  // Get the signal at that point
  signal_t value = m_ws->getSignalAtCoord(lookPoint);
//  std::cout << x << "," << y << "=" << value << "\n";
  delete [] lookPoint;
  return value;
}

QwtRasterData* QwtRasterDataMD::copy() const
{
  QwtRasterDataMD* out = new QwtRasterDataMD();
  out->m_ws = this->m_ws;
  out->m_dimX = this->m_dimX;
  out->m_dimY = this->m_dimY;
  out->m_nd = this->m_nd;
  out->m_slicePoint = new coord_t[m_nd];
  for (size_t d=0; d<m_nd; d++)
    out->m_slicePoint[d] = this->m_slicePoint[d];
  out->m_ws = this->m_ws;
  return out;
}


//------------------------------------------------------------------------------------------------------
/** Return the data range to show */
QwtDoubleInterval QwtRasterDataMD::range() const
{
  return QwtDoubleInterval(0.0, 10.0);
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
  IMDDimension_const_sptr m_X = m_ws->getDimension(m_dimX);
  IMDDimension_const_sptr m_Y = m_ws->getDimension(m_dimY);
  return QSize( 2 * int(area.width() / m_X->getBinWidth()), 2 * int(area.height() /m_Y->getBinWidth()));
}

//------------------------------------------------------------------------------------------------------
/** Sets the workspace being displayed
 *
 * @param ws :: IMDWorkspace to show
 */
void QwtRasterDataMD::setWorkspace(Mantid::API::IMDWorkspace_sptr ws)
{
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
  m_dimX = dimX;
  m_dimY = dimY;
  delete [] m_slicePoint;
  m_slicePoint = new coord_t[m_nd];
  for (size_t d=0; d<m_nd; d++)
    m_slicePoint[d] = slicePoint[d];
}
