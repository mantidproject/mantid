#ifndef IMDBOX_H_
#define IMDBOX_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/MDPoint.h"
#include "MantidMDEvents/MDDimensionExtents.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** Abstract Interface for a multi-dimensional event "box".
   * To be subclassed by MDBox and MDGridBox
   *
   * A box is a container of MDPoint's within a certain range of values
   * within the nd dimensions. This range defines a n-dimensional "box"
   * or rectangular prism.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  template <size_t nd, size_t nv = 0, typename TE = char>
  class DLLExport IMDBox
  {
  public:
    IMDBox();

    virtual void addPoint( const MDPoint<nd,nv,TE> & point) = 0;

    virtual void clear() = 0;

    virtual size_t getNPoints() const = 0;

    virtual size_t getNumDims() const = 0;

    virtual std::vector< MDPoint<nd,nv,TE> > * getPointsCopy();

    virtual double getSignal() const;

    virtual double getErrorSquared() const;

  private:

    /** Array of MDDimensionStats giving the extents and
     * other stats on the box dimensions.
     */
    MDDimensionExtents dimExtents[nd];


  public:
    /// Convenience typedef for a shared pointer to a this type of class
    typedef boost::shared_ptr< IMDBox<nd,nv,TE> > sptr;

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



}//namespace MDEvents

}//namespace Mantid

#endif /* MDBOX_H_ */
