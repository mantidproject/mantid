#ifndef BOXCONTROLLER_H_
#define BOXCONTROLLER_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadPool.h"

namespace Mantid
{
namespace MDEvents
{

  /** This class is used by MDBox and MDGridBox in order to intelligently
   * determine optimal behavior. It informs:
   *  - When a MDBox needs to split into a MDGridBox.
   *  - How the splitting will occur.
   *  - When a MDGridBox should use Tasks to parallelize adding events
   *
   * @author Janik Zikovsky
   * @date Feb 21, 2011
   */
  class DLLExport BoxController
  {
  public:

    //-----------------------------------------------------------------------------------
    /** Constructor
     *
     * @param nd :: number of dimensions
     * @return BoxController instance
     */
    BoxController(size_t nd)
    : nd(nd)
    {
      // TODO: Smarter ways to determine all of these values
      m_maxDepth = 5;
      m_addingEvents_eventsPerTask = 1000;
      m_addingEvents_numTasksPerBlock = Kernel::ThreadPool::getNumPhysicalCores() * 5;
    }

    //-----------------------------------------------------------------------------------
    /** Get # of dimensions
     * @return # of dimensions
     */
    size_t getNDims()
    {
      return nd;
    }

    //-----------------------------------------------------------------------------------
    /** Set the splitting threshold
    * @param threshold :: # of points at which the MDBox splits
    */
    void setSplitThreshold(size_t threshold)
    {
      m_SplitThreshold = threshold;
    }

    //-----------------------------------------------------------------------------------
    /** Return true if the MDBox should split, given :
     *
     * @param numPoints :: # of points in the box
     * @param depth :: recursion depth of the box
     * @return bool, true if it should split
     */
    bool willSplit(size_t numPoints, size_t depth)
    {
      return (numPoints > m_SplitThreshold) && (depth < m_maxDepth);
    }

    //-----------------------------------------------------------------------------------
    /** Return into how many to split along a dimension
     *
     * @param dim :: index of the dimension to split
     * @return the dimension will be split into this many even boxes.
     */
    size_t splitInto(size_t dim)
    {
      return m_splitInto[dim];
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done
     * @param num :: amount in which to split
     * */
    void setSplitInto(size_t num)
    {
      m_splitInto.resize(nd, num);
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done
     *
     * @param dim :: dimension to set
     * @param num :: amount in which to split
     */
    void setSplitInto(size_t dim, size_t num)
    {
      m_splitInto[dim] = num;
    }


//    //-----------------------------------------------------------------------------------
//    /** Return true if it is advantageous to use Tasks to
//     * do addEvents on a grid box.
//     *
//     * @param num :: number of events that will be added.
//     * @return bool, false if you should use a simple one-thread routine,
//     *          true if you should parallelize using tasks.
//     */
//    bool useTasksForAddingEvents(size_t num)
//    {
//      //TODO: Smarter criterion here
//      return (num > 1000);
//    }


    /** Get parameters for adding events to a MDGridBox, trying to optimize parallel CPU use.
     *
     * @param[out] eventsPerTask :: the number of events that should be added by a single task object.
     *    This should be large enough to avoid overhead without being
     *    too large, making event lists too long before splitting
     * @param[out] numTasksPerBlock :: the number of tasks (of size eventsPerTask) to be allocated
     *    before the grid boxes should be re-split. Having enough parallel tasks will
     *    help the CPU be used fully.
     */
    void getAddingEventsParameters(size_t & eventsPerTask, size_t & numTasksPerBlock)
    {
      // TODO: Smarter values here depending on nd, etc.
      eventsPerTask = m_addingEvents_eventsPerTask;
      numTasksPerBlock = m_addingEvents_numTasksPerBlock;
    }

    /** Return the max recursion depth allowed for grid box splitting. */
    size_t getMaxDepth()
    {
      return m_maxDepth;
    }


    /** Determine when would be a good time to split MDBoxes into MDGridBoxes.
     * This is to be called while adding events. Splitting boxes too frequently
     * would be a slow-down, but keeping the boxes split at an earlier stage
     * should help scalability for later adding, so there is a balance to get.
     *
     * @param eventsAdded :: How many events were added since the last split?
     * @param numMDBoxes :: How many un-split MDBoxes are there (total) in the workspace
     */
    bool shouldSplitBoxes(size_t eventsAdded, size_t numMDBoxes)
    {
      // Avoid divide by zero
      if (numMDBoxes == 0) return false;
      // Return true if the average # of events per box is big enough to split.
      return ((eventsAdded / numMDBoxes) > m_SplitThreshold);
    }


  //NOTE: These are left public for testing purposes.
  public:

    // Number of dimensions
    size_t nd;

    /// Splitting threshold
    size_t m_SplitThreshold;

    /** Maximum splitting depth: don't go further than this many levels of recursion.
     * This avoids infinite recursion and should be set to a value that gives a smallest
     * box size that is a little smaller than the finest desired binning upon viewing.
     */
    size_t m_maxDepth;

    // Even splitting for all dimensions
    std::vector<size_t> m_splitInto;

    /// For adding events tasks
    size_t m_addingEvents_eventsPerTask;
    /// For adding events tasks
    size_t m_addingEvents_numTasksPerBlock;

  };

  /// Shared ptr to BoxController
  typedef boost::shared_ptr<BoxController> BoxController_sptr;

}//namespace MDEvents

}//namespace Mantid



#endif /* SPLITCONTROLLER_H_ */
