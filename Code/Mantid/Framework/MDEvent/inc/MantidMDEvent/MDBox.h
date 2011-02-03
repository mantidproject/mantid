#ifndef MDBOX_H_
#define MDBOX_H_

#include "MantidKernel/System.h"
#include "MantidMDEvent/MDPoint.h"
#include "MantidMDEvent/MDDimensionExtents.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
namespace MDDataObjects
{




  //===============================================================================================
  /** Templated class for a multi-dimensional event "box".
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
  class DLLExport MDBox
  {
  public:
    MDBox();

    void addPoint( const MDPoint<nd,nv,TE> & point);

    void clear();

    size_t getNPoints() const;

    size_t getNumDims() const;

    std::vector< MDPoint<nd,nv,TE> > & getPoints();

    double getSignal() const;
    double getErrorSquared() const;

  private:

    /** Vector of MDEvent's, in no particular order.
     * */
    std::vector< MDPoint<nd,nv,TE> > data;

    /** Array of MDDimensionExtents giving the extents and
     * in each dimension.
     */
    MDDimensionExtents dims[nd];

    /** Total signal from all points within */
    double signal;

    /** Total error (squared) from all points within */
    double errorSquared;


  public:
    /// Typedef for a shared pointer to a MDBox
    typedef boost::shared_ptr<MDBox<nd> > sptr;

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
