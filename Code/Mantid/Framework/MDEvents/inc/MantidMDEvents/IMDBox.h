#ifndef IMDBOX_H_
#define IMDBOX_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"
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
   * A box is a container of MDEvents within a certain range of values
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
  TMDE_CLASS
  class DLLExport IMDBox
  {
  public:
    IMDBox();

    virtual void addEvent( const MDE & point) = 0;

    virtual void clear() = 0;

    virtual size_t getNPoints() const = 0;

    virtual size_t getNumDims() const = 0;

    virtual std::vector< MDE > * getEventsCopy();

    virtual double getSignal() const = 0;

    virtual double getErrorSquared() const = 0;

  private:

    /** Array of MDDimensionStats giving the extents and
     * other stats on the box dimensions.
     */
    MDDimensionExtents dimExtents[nd];


  public:
    /// Convenience typedef for a shared pointer to a this type of class
//    typedef boost::shared_ptr< IMDBox<nd> > sptr;

  };







}//namespace MDEvents

}//namespace Mantid

#endif /* IMDBOX_H_ */
