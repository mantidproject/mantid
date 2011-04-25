#ifndef MANTID_MDEVENTS_MDDIMENSIONSTATS_H_
#define MANTID_MDEVENTS_MDDIMENSIONSTATS_H_
    
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"


namespace Mantid
{
namespace MDEvents
{

  /** A simple class holding some statistics
   * on the distribution of events in a particular dimension
   * 
   * @author Janik Zikovsky
   * @date 2011-04-19 10:55:12.567192
   */
  class DLLExport MDDimensionStats 
  {
  public:
    /// Constructor
    MDDimensionStats()
    : total(0.0), totalApproxVariance(0.0), numPoints(0)
    {
    }
    
    /// Destructor (empty)
    ~MDDimensionStats()
    {  }


    //---------------------------------------------------------------------------------------
    /** Returns the mean position of events in this dimension */
    CoordType getMean() const
    { return total / double(numPoints); }

    /** Returns the approximate standard deviation of the position of events in this dimension */
    CoordType getApproxVariance() const
    { return totalApproxVariance / double(numPoints); }


    //---------------------------------------------------------------------------------------
    /** Add a point with the given coordinate; track the mean and variance
     * @param x :: coordinate value of the point in this dimension.
     */
    void addPoint(const CoordType x)
    {
      total += x;
      numPoints++;
      CoordType diff = (x - total/double(numPoints));
      totalApproxVariance += diff * diff;
    }


    //---------------------------------------------------------------------------------------
    //--- Public members ---

    /// Total dimension (summed by each event). Divide by numPoints to get the mean
    CoordType total;

    /** Approximate variance - used for quick std.deviation estimates.
     *
     * A running sum of (X - mean(X))^2, where mean(X) is calculated at the
     * time of adding the point. This approximation gets better as the number of
     * points increases.
     *
     * Divide by the number of points to get the square of the standard deviation!
     */
    CoordType totalApproxVariance;

    /// Number of points counted (used to give the mean).
    size_t numPoints;
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDDIMENSIONSTATS_H_ */
