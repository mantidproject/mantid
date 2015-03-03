#ifndef MANTID_DATAOBJECTS_MDDIMENSIONSTATS_H_
#define MANTID_DATAOBJECTS_MDDIMENSIONSTATS_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/MDLeanEvent.h"

namespace Mantid {
namespace DataObjects {

/** A simple class holding some statistics
 * on the distribution of events in a particular dimension
 *
 * @author Janik Zikovsky
 * @date 2011-04-19 10:55:12.567192
 */
class DLLExport MDDimensionStats {
public:
  /// Constructor
  MDDimensionStats() : total(0.0), totalApproxVariance(0.0), numPoints(0) {}

  /// Destructor (empty)
  ~MDDimensionStats() {}

  //---------------------------------------------------------------------------------------
  /** Returns the mean position of events in this dimension */
  coord_t getMean() const { return total / static_cast<coord_t>(numPoints); }

  /** Returns the approximate standard deviation of the position of events in
   * this dimension */
  coord_t getApproxVariance() const {
    return totalApproxVariance / static_cast<coord_t>(numPoints);
  }

  //---------------------------------------------------------------------------------------
  /** Add a point with the given coordinate; track the mean and variance
   * @param x :: coordinate value of the point in this dimension.
   */
  void addPoint(const coord_t x) {
    total += x;
    numPoints++;
    coord_t diff = (x - total / static_cast<coord_t>(numPoints));
    totalApproxVariance += diff * diff;
  }

  //---------------------------------------------------------------------------------------
  //--- Public members ---

  /// Total dimension (summed by each event). Divide by numPoints to get the
  /// mean
  coord_t total;

  /** Approximate variance - used for quick std.deviation estimates.
   *
   * A running sum of (X - mean(X))^2, where mean(X) is calculated at the
   * time of adding the point. This approximation gets better as the number of
   * points increases.
   *
   * Divide by the number of points to get the square of the standard deviation!
   */
  coord_t totalApproxVariance;

  /// Number of points counted (used to give the mean).
  size_t numPoints;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_DATAOBJECTS_MDDIMENSIONSTATS_H_ */
