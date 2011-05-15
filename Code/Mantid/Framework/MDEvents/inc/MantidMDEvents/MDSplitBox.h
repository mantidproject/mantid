#ifndef MANTID_MDEVENTS_MDSPLITBOX_H_
#define MANTID_MDEVENTS_MDSPLITBOX_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"


namespace Mantid
{
namespace MDEvents
{

  /** Similar to MDGridBox, this class is a split version of a MDBox where
   * a single left/right split occurs along a single dimension, at a variable point.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-15 10:26:16.413856
   */
  TMDE_CLASS
  class DLLExport MDSplitBox : public IMDBox<MDE,nd>
  {
  public:
    MDSplitBox(MDBox<MDE, nd> * box);

    MDSplitBox(IMDBox<MDE, nd> * box, size_t _dimSplit, CoordType _splitPoint);

    virtual ~MDSplitBox();
    
    void clear();

    size_t getNPoints() const;

    size_t getNumDims() const;

    size_t getNumMDBoxes() const;

    void addEvent(const MDE & point);

    void splitContents(size_t index, Mantid::Kernel::ThreadScheduler * ts);

    void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * ts);

    void refreshCache(Kernel::ThreadScheduler * ts = NULL);

    virtual std::vector< MDE > * getEventsCopy()
    {return NULL;}

    virtual void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const;

    void integrateSphere(CoordTransform & /*radiusTransform*/, const CoordType /*radiusSquared*/, double & /*signal*/, double & /*errorSquared*/) const
    { throw std::runtime_error("Not implemented."); }

    // --------------------------------------------------------------------------------------------

    /// Return which dimension (index) was split
    size_t getSplitDimension() const
    { return dimSplit; }

    /// Return the X value in the split dimension that was the left/right splitting point
    CoordType getSplitPoint() const
    { return splitPoint; }

    /// Returns the IMDBox on the left side (x[dimSplit] < splitPoint)
    IMDBox<MDE,nd> * getLeft()
    { return left; }

    /// Returns the IMDBox on the right side (x[dimSplit] >= splitPoint)
    IMDBox<MDE,nd> * getRight()
    { return right; }

  protected:
    /// Total number of points (events) in all sub-boxes
    size_t nPoints;

    /// Index of the dimension that this MDSplitBox splits. Between 0 and nd.
    size_t dimSplit;

    /// X-value that splits the dimension at index dimSplit.
    CoordType splitPoint;

    /// IMDBox on the left of the split (x[dimSplit] < splitPoint)
    IMDBox<MDE,nd> * left;

    /// IMDBox on the right of the split (x[dimSplit] >= splitPoint)
    IMDBox<MDE,nd> * right;

  private:
    /// Used by constructor only.
    void initBoxes(IMDBox<MDE, nd> * box);

  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDSPLITBOX_H_ */
