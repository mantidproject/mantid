#ifndef BOXSPLITCONTROLLER_H_
#define BOXSPLITCONTROLLER_H_

#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace MDEvents
{

  /** This class is used by MDBox and MDGridBox to determine when,
   * a MDBox needs to split into a MDGridBox.
   * It also specifies the way the splitting will occur.
   *
   * @author Janik Zikovsky
   * @date Feb 21, 2011
   */
  class BoxSplitController
  {
  public:

    //-----------------------------------------------------------------------------------
    /** Constructor
     *
     * @param threshold :: # of points at which the MDBox splits
     * @return
     */
    BoxSplitController(size_t threshold)
    : m_threshold(threshold)
    {
    }

    //-----------------------------------------------------------------------------------
    /** Return true if the MDBox should split, given :
     *
     * @param original :: current size (# of points) in the box
     * @param added :: # to add
     * @return bool
     */
    bool willSplit(size_t original, size_t added)
    {
      return (original+added) > m_threshold;
    }

    //-----------------------------------------------------------------------------------
    /** Return into how many to split along a dimension
     *
     * @param dim :: index of the dimension to split
     * @return the dimension will be split into this many even boxes.
     */
    size_t splitInto(size_t dim)
    {
      (void) dim; // Ignore dim for now
      return m_splitInto;
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done */
    void setSplitInto(size_t num)
    {
      m_splitInto = num;
    }


  protected:
    /// Splitting threshold
    size_t m_threshold;

    // Even splitting for all dimensions
    size_t m_splitInto;

  };

  /// Shared ptr to BoxSplitController
  typedef boost::shared_ptr<BoxSplitController> BoxSplitController_sptr;

}//namespace MDEvents

}//namespace Mantid



#endif /* SPLITCONTROLLER_H_ */
