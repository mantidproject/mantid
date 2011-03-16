#ifndef MDBOX_H_
#define MDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDDimensionExtents.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** Templated class for a multi-dimensional event "box".
   *
   * A box is a container of MDEvent's within a certain range of values
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
  TMDE_CLASS
  class DLLExport MDBox : public IMDBox<MDE, nd>
  {
  public:
    MDBox();

    MDBox(BoxController_sptr splitter);

    void clear();

    size_t getNPoints() const;

    size_t getNumDims() const;

    std::vector< MDE > & getEvents();

    std::vector< MDE > * getEventsCopy();

    size_t addEvent(const MDE & point);

    size_t addEvents(const std::vector<MDE> & events);

    bool willSplit(size_t num) const;

  protected:

    /** Vector of MDEvent's, in no particular order.
     * */
    std::vector< MDE > data;


  public:
    /// Typedef for a shared pointer to a MDBox
    typedef boost::shared_ptr< MDBox<MDE, nd> > sptr;

    /// Typedef for a vector of the conatined events
    typedef std::vector< MDE > vec_t;

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
