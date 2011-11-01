#include "MantidQtSliceViewer/QwtRasterDataMD.h"
#include <math.h>
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid;
using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

//-------------------------------------------------------------------------
/// Constructor
QwtRasterDataMD::QwtRasterDataMD()
: m_slicePoint(NULL)
{
  timesRequested = 0;
  m_minVal = DBL_MAX;
  m_maxVal = -DBL_MAX;
  m_range = QwtDoubleInterval(0.0, 1.0);
  m_logMode = true;
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
  out->m_logMode = this->m_logMode;
  out->m_dimX = this->m_dimX;
  out->m_dimY = this->m_dimY;
  out->m_nd = this->m_nd;
  out->m_minVal = this->m_minVal;
  out->m_maxVal = this->m_maxVal;
  out->m_range = this->m_range;
  out->m_slicePoint = new coord_t[m_nd];
  for (size_t d=0; d<m_nd; d++)
    out->m_slicePoint[d] = this->m_slicePoint[d];
  out->m_ws = this->m_ws;
  return out;
}

//-------------------------------------------------------------------------
/** Set the data range (min/max) to display */
void QwtRasterDataMD::setRange(const QwtDoubleInterval & range)
{ m_range = range; }

//-------------------------------------------------------------------------
/** Sets whether to show the log10 of the data
 * @param log :: true to use log color scaling.
 */
void QwtRasterDataMD::setLogMode(bool log)
{ m_logMode = log;
}


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
      lookPoint[d] = coord_t(x);
    else if (d==m_dimY)
      lookPoint[d] = coord_t(y);
    else
      lookPoint[d] = m_slicePoint[d];
  }
  // Get the signal at that point
  signal_t value = m_ws->getSignalAtCoord(lookPoint);
  if (value < m_minVal) m_minVal = value;
  if (value > m_maxVal) m_maxVal = value;
  delete [] lookPoint;

  if (m_logMode)
  {
    if (value <= 0.)
      return nan;
    else
      return log10(value);
  }
  else
  {
    return value;
  }
}


//------------------------------------------------------------------------------------------------------
/** Return the data range to show */
QwtDoubleInterval QwtRasterDataMD::range() const
{
  if (m_logMode)
  {
    double min = log10(m_range.minValue());
    double max = log10(m_range.maxValue());
    if (m_range.minValue() <= 0)
      min = max-6;
    return QwtDoubleInterval(min,max);
  }
  else
    // Linear color plot
    return m_range;

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
