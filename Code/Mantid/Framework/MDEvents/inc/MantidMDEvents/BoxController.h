#ifndef BOXCONTROLLER_H_
#define BOXCONTROLLER_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/MultiThreaded.h"

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
  class BoxController
  {
  public:
    //-----------------------------------------------------------------------------------
    /** Constructor
     *
     * @param nd :: number of dimensions
     * @return BoxController instance
     */
    BoxController(size_t nd)
    :nd(nd)
    {
      // TODO: Smarter ways to determine all of these values
      m_maxDepth = 5;
      m_addingEvents_eventsPerTask = 1000;
      m_addingEvents_numTasksPerBlock = Kernel::ThreadPool::getNumPhysicalCores() * 5;
      resetNumBoxes();
    }

    //-----------------------------------------------------------------------------------
    /** Get # of dimensions
     * @return # of dimensions
     */
    size_t getNDims() const
    {
      return nd;
    }

    //-----------------------------------------------------------------------------------
    /** Return true if the MDBox should split, given :
     *
     * @param numPoints :: # of points in the box
     * @param depth :: recursion depth of the box
     * @return bool, true if it should split
     */
    bool willSplit(size_t numPoints, size_t depth) const
    {
      return (numPoints > m_SplitThreshold) && (depth < m_maxDepth);
    }

    //-----------------------------------------------------------------------------------
    /** Return the splitting threshold, in # of events */
    size_t getSplitThreshold() const
    {
      return m_SplitThreshold;
    }

    /** Set the splitting threshold
     * @param threshold :: # of points at which the MDBox splits
     */
    void setSplitThreshold(size_t threshold)
    {
      m_SplitThreshold = threshold;
    }

    //-----------------------------------------------------------------------------------
    /** Return into how many to split along a dimension
     *
     * @param dim :: index of the dimension to split
     * @return the dimension will be split into this many even boxes.
     */
    size_t getSplitInto(size_t dim) const
    {
      return m_splitInto[dim];
    }

    /// Return how many boxes (total) a MDGridBox will contain.
    size_t getNumSplit() const
    {
      return m_numSplit;
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done
     * @param num :: amount in which to split
     * */
    void setSplitInto(size_t num)
    {
      m_splitInto.resize(nd, num);
      calcNumSplit();
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
      calcNumSplit();
    }

    //-----------------------------------------------------------------------------------
    /// Getters/setters
    size_t getAddingEvents_eventsPerTask() const
    {
      return m_addingEvents_eventsPerTask;
    }

    void setAddingEvents_eventsPerTask(size_t m_addingEvents_eventsPerTask)
    {
      this->m_addingEvents_eventsPerTask = m_addingEvents_eventsPerTask;
    }

    void setAddingEvents_numTasksPerBlock(size_t m_addingEvents_numTasksPerBlock)
    {
      this->m_addingEvents_numTasksPerBlock = m_addingEvents_numTasksPerBlock;
    }

    size_t getAddingEvents_numTasksPerBlock() const
    {
      return m_addingEvents_numTasksPerBlock;
    }


    //-----------------------------------------------------------------------------------
    /** Get parameters for adding events to a MDGridBox, trying to optimize parallel CPU use.
     *
     * @param[out] eventsPerTask :: the number of events that should be added by a single task object.
     *    This should be large enough to avoid overhead without being
     *    too large, making event lists too long before splitting
     * @param[out] numTasksPerBlock :: the number of tasks (of size eventsPerTask) to be allocated
     *    before the grid boxes should be re-split. Having enough parallel tasks will
     *    help the CPU be used fully.
     */
    void getAddingEventsParameters(size_t & eventsPerTask, size_t & numTasksPerBlock) const
    {
      // TODO: Smarter values here depending on nd, etc.
      eventsPerTask = m_addingEvents_eventsPerTask;
      numTasksPerBlock = m_addingEvents_numTasksPerBlock;
    }


    //-----------------------------------------------------------------------------------
    /** Return the max recursion depth allowed for grid box splitting. */
    size_t getMaxDepth() const
    {
      return m_maxDepth;
    }

    /** Sets the max recursion depth allowed for grid box splitting.
     * NOTE! This resets numMDBoxes stats!
     *  */
    void setMaxDepth(size_t value)
    {
      m_maxDepth = value;
      resetNumBoxes();
    }

    //-----------------------------------------------------------------------------------
    /** Determine when would be a good time to split MDBoxes into MDGridBoxes.
     * This is to be called while adding events. Splitting boxes too frequently
     * would be a slow-down, but keeping the boxes split at an earlier stage
     * should help scalability for later adding, so there is a balance to get.
     *
     * @param eventsAdded :: How many events were added since the last split?
     * @param numMDBoxes :: How many un-split MDBoxes are there (total) in the workspace
     */
    bool shouldSplitBoxes(size_t eventsAdded, size_t numMDBoxes) const
    {
      // Avoid divide by zero
      if(numMDBoxes == 0)
        return false;

      // Return true if the average # of events per box is big enough to split.
      return ((eventsAdded / numMDBoxes) > m_SplitThreshold);
    }


    //-----------------------------------------------------------------------------------
    /** Call to track the number of MDBoxes are contained in the MDEventWorkspace
     * This should be called when a MDBox gets split into a MDGridBox.
     * The number of MDBoxes at [depth] is reduced by one
     * The number of MDBoxes at [depth+1] is increased by however many the splitting gives.
     *
     * @param depth :: the depth of the MDBox that is being split into MDGrid boxes.
     */
    void trackNumBoxes(size_t depth)
    {
      m_mutexNumMDBoxes.lock();
      if (m_numMDBoxes[depth] > 0)
        m_numMDBoxes[depth]--;
      m_numMDBoxes[depth + 1] += m_numSplit;
      m_mutexNumMDBoxes.unlock();
    }

    /** Return the vector giving the number of MD Boxes as a function of depth */
    const std::vector<size_t> & getNumMDBoxes() const
    {
      return m_numMDBoxes;
    }

    /** Return the total number of MD Boxes, irrespective of depth */
    size_t getTotalNumMDBoxes() const
    {
      size_t total = 0;
      for (size_t d=0; d<nd; d++)
      {
        total += m_numMDBoxes[d];
      }
      return total;
    }

    /** Reset the number of boxes tracked in m_numMDBoxes */
    void resetNumBoxes()
    {
      m_mutexNumMDBoxes.lock();
      m_numMDBoxes.clear();
      m_numMDBoxes.resize(m_maxDepth + 1, 0); // Reset to 0
      m_maxNumMDBoxes.resize(m_maxDepth + 1, 0); // Reset to 0
      m_numMDBoxes[0] = 1; // Start at 1 at depth 0.
      // Now calculate the max # of boxes
      m_maxNumMDBoxes[0] = 1;
      for (size_t d=1; d<m_maxNumMDBoxes.size(); d++)
        m_maxNumMDBoxes[d] = m_maxNumMDBoxes[d-1] * m_numSplit;
      m_mutexNumMDBoxes.unlock();
    }



    //-----------------------------------------------------------------------------------
  private:
    /// When you split a MDBox, it becomes this many sub-boxes
    void calcNumSplit()
    {
      m_numSplit = 1;
      for(size_t d = 0;d < nd;d++){
        m_numSplit *= m_splitInto[d];
      }
    }

    /// Number of dimensions
    size_t nd;

    /// Splitting threshold
    size_t m_SplitThreshold;

    /** Maximum splitting depth: don't go further than this many levels of recursion.
     * This avoids infinite recursion and should be set to a value that gives a smallest
     * box size that is a little smaller than the finest desired binning upon viewing.
     */
    size_t m_maxDepth;

    /// Splitting # for all dimensions
    std::vector<size_t> m_splitInto;

    /// When you split a MDBox, it becomes this many sub-boxes
    size_t m_numSplit;

    /// For adding events tasks
    size_t m_addingEvents_eventsPerTask;

    /// For adding events tasks
    size_t m_addingEvents_numTasksPerBlock;

    /// For tracking how many MDBoxes (not MDGridBoxes) are at each recursion level
    std::vector<size_t> m_numMDBoxes;

    /// Mutex for changing the number of MD Boxes.
    Mantid::Kernel::Mutex m_mutexNumMDBoxes;

    /// This is the maximum number of MD boxes there could be at each recursion level (e.g. (splitInto ^ ndims) ^ depth )
    std::vector<size_t> m_maxNumMDBoxes;

  };

  /// Shared ptr to BoxController
  typedef boost::shared_ptr<BoxController> BoxController_sptr;

}//namespace MDEvents

}//namespace Mantid



#endif /* SPLITCONTROLLER_H_ */
