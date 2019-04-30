// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/QwtRasterDataMDNonOrthogonal.h"
#include "MantidQtWidgets/Common/NonOrthogonal.h"

namespace MantidQt {
namespace API {

using namespace Mantid;
using namespace Mantid::API;

QwtRasterDataMDNonOrthogonal::QwtRasterDataMDNonOrthogonal()
    : m_lookPoint(),
      m_fromHklToXyz({{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}}),
      m_missingHKLdim(0) {}

//-------------------------------------------------------------------------
/** Return the data value to plot at the given position
 *
 * @param x :: position in coordinates of the MDWorkspace
 * @param y :: position in coordinates of the MDWorkspace
 * @return signal to plot
 */
double QwtRasterDataMDNonOrthogonal::value(double x, double y) const {
  if (!m_ws)
    return 0;

  // Generate the vector of coordinates, filling in X and Y
  for (size_t d = 0; d < m_nd; d++) {
    if (d == m_dimX)
      m_lookPoint[d] = static_cast<coord_t>(x);
    else if (d == m_dimY)
      m_lookPoint[d] = static_cast<coord_t>(y);
    else {
      m_lookPoint[d] = m_slicePoint[d];
    }
  }

  // Transform the lookpoint to the coordinate of the workspace
  transformLookpointToWorkspaceCoord(m_lookPoint, m_fromHklToXyz, m_dimX,
                                     m_dimY, m_missingHKLdim);
  // Get the signal at that point
  signal_t value = 0;

  // Check if the overlay WS is within range of being viewed
  if (m_overlayWS && m_overlayInSlice && (x >= m_overlayXMin) &&
      (x < m_overlayXMax) && (y >= m_overlayYMin) && (y < m_overlayYMax)) {
    // Point is in the overlaid workspace
    value = m_overlayWS->getSignalWithMaskAtCoord(m_lookPoint.data(),
                                                  m_normalization);
  } else {
    // No overlay, or not within range of that workspace
    value = m_ws->getSignalWithMaskAtCoord(m_lookPoint.data(), m_normalization);
  }

  // Special case for 0 = show as NAN
  if (m_zerosAsNan && value == 0.)
    return nan;

  return value;
}

//------------------------------------------------------------------------------------------------------
/** Sets the workspace being displayed
 *
 * @param ws :: IMDWorkspace to show
 */
void QwtRasterDataMDNonOrthogonal::setWorkspace(IMDWorkspace_const_sptr ws) {
  QwtRasterDataMD::setWorkspace(ws);
  // Create a lookpoint
  m_lookPoint.resize(m_nd);
  // Add the skewMatrix for the basis
  Mantid::Kernel::DblMatrix skewMatrix(m_nd, m_nd, true);
  provideSkewMatrix(skewMatrix, *ws);
  transformFromDoubleToCoordT(skewMatrix, m_fromHklToXyz);
}

void QwtRasterDataMDNonOrthogonal::setSliceParams(
    size_t dimX, size_t dimY, Mantid::Geometry::IMDDimension_const_sptr X,
    Mantid::Geometry::IMDDimension_const_sptr Y,
    std::vector<Mantid::coord_t> &slicePoint) {
  QwtRasterDataMD::setSliceParams(dimX, dimY, X, Y, slicePoint);
  // find missing HKL
  m_missingHKLdim = API::getMissingHKLDimensionIndex(m_ws, dimX, dimY);
}

//-------------------------------------------------------------------------
/** Perform a copy of this data object */
QwtRasterDataMDNonOrthogonal *QwtRasterDataMDNonOrthogonal::copy() const {
  QwtRasterDataMDNonOrthogonal *out = new QwtRasterDataMDNonOrthogonal();
  this->copyFrom(*this, *out);
  return out;
}

/**
 * Copy settings from one object to another
 * @param source A source object to copy from
 * @param dest The destination object that receives the contents
 */
void QwtRasterDataMDNonOrthogonal::copyFrom(
    const QwtRasterDataMDNonOrthogonal &source,
    QwtRasterDataMDNonOrthogonal &dest) const {
  // base bounding box
  dest.setBoundingRect(source.boundingRect());

  dest.m_ws = source.m_ws;
  dest.m_dimX = source.m_dimX;
  dest.m_dimY = source.m_dimY;
  dest.m_nd = source.m_nd;
  dest.m_range = source.m_range;
  dest.m_slicePoint = new coord_t[m_nd];
  for (size_t d = 0; d < m_nd; d++)
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
  dest.m_missingHKLdim = source.m_missingHKLdim;
  dest.m_lookPoint = source.m_lookPoint;

  std::copy(std::begin(source.m_fromHklToXyz), std::end(source.m_fromHklToXyz),
            std::begin(dest.m_fromHklToXyz));
}

} // namespace API
} // namespace MantidQt
