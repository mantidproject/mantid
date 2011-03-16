#ifndef MDGRIDBOX_H_
#define MDGRIDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDDimensionExtents.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** Templated class for a GRIDDED multi-dimensional event "box".
   * A MDGridBox contains a dense array with nd dimensions
   * of IMDBox'es, each being either a regular MDBox or a MDGridBox itself.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDGridBox : public IMDBox<MDE, nd>
  {
  public:
    MDGridBox(MDBox<MDE, nd> * box);

    void clear();

    size_t getNPoints() const;

    size_t getNumDims() const;

    std::vector< MDE > * getEventsCopy();

    size_t addEvent(const MDE & point);

    size_t addEvents(const std::vector<MDE> & events);

    bool willSplit(size_t num) const;

    void splitContents(size_t index);

    // ======================= Testing/Debugging Methods =================
    /** For testing: get the vector of boxes */
    std::vector<IMDBox<MDE, nd>*> getBoxes()
    { return boxes; }

  private:

    /** Array of MDDimensionExtents giving the extents and
     * in each dimension.
     */
    MDDimensionExtents extents[nd];

    /// Each dimension is split into this many equally-sized boxes
    size_t split[nd];

    /** Cumulative dimension splitting: split[n] = 1*split[0]*split[..]*split[n-1]
     */
    size_t splitCumul[nd];

    /** 1D array of boxes contained within. These map
     * to the nd-array.
     */
    std::vector<IMDBox<MDE, nd>*> boxes;

    /// Size of each box size in the i^th dimension
    CoordType boxSize[nd];

    /// Cached number of points contained (including all sub-boxes)
    size_t nPoints;


  public:
    /// Typedef for a shared pointer to a MDGridBox
    typedef boost::shared_ptr< MDGridBox<MDE, nd> > sptr;

  };






}//namespace MDEvents

}//namespace Mantid

#endif /* MDGRIDBOX_H_ */
