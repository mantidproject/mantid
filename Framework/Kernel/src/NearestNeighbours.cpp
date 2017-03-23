
#include "MantidKernel/NearestNeighbours.h"

using namespace Mantid::Kernel;

/** Construct a new set of data points
   *
   * @param nPts :: the number of data points
   * @param nElems :: the number of elements for each point
   */
NNDataPoints::NNDataPoints(const size_t nPts, const size_t nElems)
    : m_nPts(nPts) {
  m_data = annAllocPts(static_cast<int>(m_nPts), static_cast<int>(nElems));
}

NNDataPoints::~NNDataPoints() { annDeallocPts(m_data); }

/** Return a handle to the raw ANNpointArray wrapped by this class
   *
   * @return handle to the raw ANNpointArray
   */
ANNpointArray NNDataPoints::rawData() { return m_data; }

/** Access a raw point in the collection of points
   *
   * This will check the index used is within bounds and return nullptr if
   * outside of those bounds
   *
   * @param i :: the index of the point to return a handle to
   * @return handle to a single point in the collection of points
   */
ANNcoord *NNDataPoints::mutablePoint(const size_t i) {
  if (i < m_nPts)
    return m_data[i];
  else
    return nullptr;
}
