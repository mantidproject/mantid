#ifndef MDGRIDBOX_H_
#define MDGRIDBOX_H_

#include "MantidKernel/System.h"
#include "MantidMDEvent/MDPoint.h"
#include "MantidMDEvent/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace MDDataObjects
{
  // Forward declaration
  class MDDimensionStats;


  //===============================================================================================
  /** Class for a gridded box that contains other boxes.
   *
   * A box is a container of MDPoint's within a certain range of values
   * within the nd dimensions. This range defines a n-dimensional "box"
   * or rectangular prism.
   *
   * This class is a simple list of points with no more internal structure.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  template <size_t nd, size_t nv = 0, typename TE = char[0]>
  class DLLExport MDGridBox
  {
  public:
    MDGridBox();

    virtual void addPoint( const MDPoint<nd,nv,TE> & point);

    virtual void clear();

    virtual MDDimensionStats getStats(const size_t dim) const;

    virtual size_t getNPoints() const;

    size_t getNumDims() const;

    std::vector< MDPoint<nd,nv,TE> > & getPoints();

    double getSignal() const;
    double getErrorSquared() const;

  private:

    /** Vector of MDEvent's, in no particular order.
     * */
    std::vector< MDPoint<nd,nv,TE> > data;

    /** Array of MDDimensionStats giving the extents and
     * other stats on the box dimensions.
     */
    MDDimensionStats dimStats[nd];

    /** Total signal from all points within */
    double signal;

    /** Total error (squared) from all points within */
    double errorSquared;


  public:
    /// Typedef for a shared pointer to a MDBox
    typedef boost::shared_ptr<MDBox<nd> > sptr;

  };










  //===============================================================================================
  /** Simple class which holds the extents (min/max)
   * of a given dimension in a MD workspace or MDBox
   */
  DLLExport class MDDimensionExtents
  {
  public:

    /** Empty constructor - reset everything */
    MDDimensionExtents() :
      min( std::numeric_limits<CoordType>::max() ),
      max( -std::numeric_limits<CoordType>::max() )
    { }

    // ---- Public members ----------
    /// Extent: minimum value in that dimension
    CoordType min;
    /// Extent: maximum value in that dimension
    CoordType max;
  };




  //===============================================================================================
  /** Simple class which holds statistics
   * about a given dimension in a MD workspace or MDBox
   */
  DLLExport class MDDimensionStats : public MDDimensionExtents
  {
  public:

    /** Empty constructor - reset everything */
    MDDimensionStats() :
      MDDimensionExtents(),
      total( 0 ),
      approxVariance( 0 )
    { }

    // ---- Public members ----------

    /** Sum of the coordinate value of all points contained.
     * Divide by the number of points to get the mean!
     */
    CoordType total;

    /** Approximate variance - used for quick std.deviation estimates.
     *
     * A running sum of (X - mean(X))^2, where mean(X) is calculated at the
     * time of adding the point. This approximation gets better as the number of
     * points increases.
     *
     * Divide by the number of points to get the square of the standard deviation!
     */
    CoordType approxVariance;
  };



}//namespace MDDataObjects

}//namespace Mantid

#endif /* MDBOX_H_ */
