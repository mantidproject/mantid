#include "MantidAPI/Progress.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDGridBox.h"
#include <ostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{


  //===============================================================================================
  //===============================================================================================
  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param box :: MDBox containing the events to split */
  TMDE(MDGridBox)::MDGridBox(MDBox<MDE, nd> * box) :
    IMDBox<MDE, nd>(),
    nPoints(0)
  {
    if (!box)
      throw std::runtime_error("MDGridBox::ctor(): box is NULL.");
    BoxController_sptr bc = box->getBoxController();
    if (!bc)
      throw std::runtime_error("MDGridBox::ctor(): No BoxController specified in box.");
    // Save the controller in this object.
    this->m_BoxController = bc;

    // Copy the extents
    for (size_t d=0; d<nd; d++)
      extents[d] = box->getExtents(d);
    // Copy the depth
    this->m_depth = box->getDepth();

    // Do some computation based on how many splits per each dim.
    size_t tot = 1;
    for (size_t d=0; d<nd; d++)
    {
      // Cumulative multiplier, for indexing
      splitCumul[d] = tot;
      // How many is it split?
      split[d] = bc->splitInto(d);
      tot *= split[d];
      // Length of the side of a box in this dimension
      boxSize[d] = (extents[d].max - extents[d].min) / split[d];
    }

    if (tot == 0)
      throw std::runtime_error("MDGridBox::ctor(): Invalid splitting criterion (one was zero).");

    // Create the array of MDBox contents.
    boxes.clear();
    boxes.reserve(tot);
    numBoxes = tot;

    size_t indices[nd];
    for (size_t d=0; d<nd; d++) indices[d] = 0;
    for (size_t i=0; i<tot; i++)
    {
      // Create the box
      // (Increase the depth of this box to one more than the parent (this))
      MDBox<MDE,nd> * myBox = new MDBox<MDE,nd>(bc, this->m_depth + 1);
      // Set the extents of this box.
      for (size_t d=0; d<nd; d++)
      {
        CoordType min = extents[d].min + boxSize[d] * indices[d];
        myBox->setExtents(d, min, min + boxSize[d]);
      }
      boxes.push_back(myBox);

      // Increment the indices, rolling back as needed
      indices[0]++;
      for (size_t d=0; d<nd-1; d++) //This is not run if nd=1; that's okay, you can ignore the warning
      {
        if (indices[d] >= split[d])
        {
          indices[d] = 0;
          indices[d+1]++;
        }
      }
    } // for each box

    // Now distribute the events that were in the box before
    this->addEvents(box->getEvents());
    // Copy the cached numbers from the incoming box. This is most efficient
    this->nPoints = box->getNPoints();
    this->m_signal = box->getSignal();
    this->m_errorSquared = box->getErrorSquared();
  }


  //-----------------------------------------------------------------------------------------------
  /// Destructor
  TMDE(MDGridBox)::~MDGridBox()
  {
    // Delete all contained boxes (this should fire the MDGridBox destructors recursively).
    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
      delete *it;
    boxes.clear();
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDGridBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
    {
      (*it)->clear();
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDGridBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDGridBox)::getNPoints() const
  {
    //Use the cached value
    return nPoints;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of un-split MDBoxes in this box (including all children)
   * @return :: the total # of MDBoxes in all children */
  TMDE(
  size_t MDGridBox)::getNumMDBoxes() const
  {
    size_t total = 0;
    typename boxVector_t::const_iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
    {
      total += (*it)->getNumMDBoxes();
    }
    return total;
  }


  //-----------------------------------------------------------------------------------------------
  /** Helper function to get the index into the linear array given
   * an array of indices for each dimension (0 to nd)
   * @param indices :: array of size[nd]
   * @return size_t index into boxes[].
   */
  TMDE(
  inline size_t MDGridBox)::getLinearIndex(size_t * indices) const
  {
    size_t out_linear_index = 0;
    for (size_t d=0; d<nd; d++)
      out_linear_index += (indices[d] * splitCumul[d]);
    return out_linear_index;
  }

  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache of nPoints, signal and error,
   * by adding up all boxes (recursively).
   * MDBoxes' totals are used directly.
   */
  TMDE(
  void MDGridBox)::refreshCache()
  {
    // Clear your total
    nPoints = 0;
    this->m_signal = 0;
    this->m_errorSquared = 0;

    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); it++)
    {
      IMDBox<MDE,nd> * ibox = *it;
      MDGridBox<MDE,nd> * gbox = dynamic_cast<MDGridBox<MDE,nd> *>(ibox);

      // Refresh the cache of the contents. Recursive.
      if (gbox)
        gbox->refreshCache();

      // Add up what's in there
      nPoints += ibox->getNPoints();
      this->m_signal += ibox->getSignal();
      this->m_errorSquared += ibox->getErrorSquared();
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDGridBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    //out->insert(out->begin(), data.begin(), data.end());
    return out;
  }


  //-----------------------------------------------------------------------------------------------
  /** Split a box that is contained in the GridBox, at the given index,
   * into a MDGridBox.
   *
   * Thread-safe as long as 'index' is different for all threads.
   *
   * @param index :: index into the boxes vector.
   *        Warning: No bounds check is made, don't give stupid values!
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL for no recursive splitting.
   */
  TMDE(
  void MDGridBox)::splitContents(size_t index, ThreadScheduler * ts)
  {
    // You can only split it if it is a MDBox (not MDGridBox).
    MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[index]);
    if (!box) return;
    // Construct the grid box
    MDGridBox<MDE, nd> * gridbox = new MDGridBox<MDE, nd>(box);
    // Delete the old ungridded box
    delete boxes[index];
    // And now we have a gridded box instead of a boring old regular box.
    boxes[index] = gridbox;

    if (ts)
    {
      // Create a task to split the newly create MDGridBox.
      ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitAllIfNeeded, &*gridbox, ts) ) );
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Goes through all the sub-boxes and splits them if they contain
   * enough events to be worth it.
   *
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL to do it serially.
   */
  TMDE(
  void MDGridBox)::splitAllIfNeeded(ThreadScheduler * ts)
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
      if (box)
      {
        // Plain MD-Box. Does it need to split?
        if (this->m_BoxController->willSplit(box->getNPoints(), box->getDepth() ))
        {
          // The MDBox needs to split into a grid box.
          if (!ts)
          {
            // ------ Perform split serially (no ThreadPool) ------
            MDGridBox<MDE, nd> * gridBox = new MDGridBox<MDE, nd>(box);
            // Replace in the array
            boxes[i] = gridBox;
            // Delete the old box
            delete box;
            // Now recursively check if this NEW grid box's contents should be split too
            gridBox->splitAllIfNeeded(NULL);
          }
          else
          {
            // ------ Perform split in parallel (using ThreadPool) ------
            // So we create a task to split this MDBox,
            // Task is : this->splitContents(i, ts);
            ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitContents, &*this, i, ts) ) );
          }
        }
      }
      else
      {
        // It should be a MDGridBox
        MDGridBox<MDE, nd> * gridBox = dynamic_cast<MDGridBox<MDE, nd>*>(boxes[i]);
        if (gridBox)
        {
          // Now recursively check if this old grid box's contents should be split too
          if (!ts || (this->nPoints < this->m_BoxController->m_addingEvents_eventsPerTask))
            // Go serially if there are only a few points contained (less overhead).
            gridBox->splitAllIfNeeded(ts);
          else
            // Go parallel if this is a big enough gridbox.
            // Task is : gridBox->splitAllIfNeeded(ts);
            ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitAllIfNeeded, &*gridBox, ts) ) );
        }
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a single MDEvent to the grid box. If the boxes
   * contained within are also gridded, this will recursively push the event
   * down to the deepest level.
   * Warning! No bounds checking is done (for performance). It must
   * be known that the event is within the bounds of the grid box before adding.
   *
   * Note! nPoints, signal and error must be re-calculated using refreshCache()
   * after all events have been added.
   *
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  inline void MDGridBox)::addEvent( const MDE & event)
  {
    size_t index = 0;
    for (size_t d=0; d<nd; d++)
    {
      CoordType x = event.getCenter(d);
      int i = int((x - extents[d].min) / boxSize[d]);
      // NOTE: No bounds checking is done (for performance).
      //if (i < 0 || i >= int(split[d])) return;

      // Accumulate the index
      index += (i * splitCumul[d]);
    }

    // Add it to the contained box, and return how many bads where there
    boxes[index]->addEvent(event);
  }



  //-----------------------------------------------------------------------------------------------
  /** Add several events.
   *
   * @param events :: vector of events to be copied.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDGridBox)::addEvents(const std::vector<MDE> & events)
  {
    return this->addEvents(events, 0, events.size());
  }

  //-----------------------------------------------------------------------------------------------
  /** Add several events, starting and stopping at particular point in a vector.
   * Bounds checking IS performed, and events outside the range are rejected.
   *
   * NOTE: You must call refreshCache() after you are done, to calculate the
   *  nPoints, signal and error.
   *
   * @param events :: vector of events to be copied.
   * @param start_at :: begin at this index in the array
   * @param stop_at :: stop at this index in the array
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDGridBox)::addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
  {
    size_t numBad = 0;
    // --- Go event by event and add them ----
    typename std::vector<MDE>::const_iterator it = events.begin() + start_at;
    typename std::vector<MDE>::const_iterator it_end = events.begin() + stop_at;
    for (; it != it_end; it++)
    {
      //Check out-of-bounds-ness
      bool badEvent = false;
      for (size_t d=0; d<nd; d++)
      {
        double x = it->getCenter(d);
        if ((x < extents[d].min) || (x >= extents[d].max))
        {
          badEvent = true;
          break;
        }
      }

      if (badEvent)
        // Event was out of bounds. Count it
        ++numBad;
      else
        // Event was in bounds; add it
        addEvent(*it);
    }

    return numBad;
  }



  //-----------------------------------------------------------------------------------------------
  /** Perform centerpoint binning of events.
   * @param bin :: MDBin object giving the limits of events to accept.
   */
  TMDE(
  void MDGridBox)::centerpointBin(MDBin<MDE,nd> & bin) const
  {
//    // We have to find boxes that are partially contained, and those fully contained.
//    for (size_t i=0; i < numBoxes; i++)
//    {
//      // Look at this box
//      IMDBox<MDE,nd> * box = this->boxes[i];
//
//      // Count of dimensions where the box's "min" is contained within the bin
//      size_t dims_min_contained = 0;
//      // Count of dimensions where the box's "max" is contained
//      size_t dims_max_contained = 0;
//
//      for (size_t d=0; d<nd; d++)
//      {
//        CoordType binMin = bin.m_min[d];
//        CoordType binMax = bin.m_max[d];
//        MDDimensionExtents & extents = box->getExtents(d);
//        if ((extents.min >= binMin) && (extents.min <= binMax))
//          dims_min_contained++;
//
//        if ((extents.max >= binMin) && (extents.max <= binMax))
//          dims_max_contained++;
//      }
//
//      size_t any_contained = (dims_min_contained + dims_max_contained);
//      if (any_contained == 2*nd)
//      {
//        // The box is fully contained (the min and max of each dimension is within the bin limits)!
//      }
//      else if (any_contained > 0)
//      {
//        // The box is partly contained.
//      }
//    }


    // The MDBin ranges from index_min to index_max (inclusively) if each dimension. So
    // we'll need to make nested loops from index_min[0] to index_max[0]; from index_min[1] to index_max[1]; etc.
    int index_min[nd];
    int index_max[nd];
    // For running the nested loop, counters of each dimension. These are bounded by 0..split[d]-1
    size_t counters_min[nd];
    size_t counters_max[nd];

    for (size_t d=0; d<nd; d++)
    {
      int min,max;

      // The min index in this dimension (we round down - we'll include this edge)
      if (bin.m_min[d] >= extents[d].min)
      {
        min = size_t((bin.m_min[d] - extents[d].min) / boxSize[d]);
        counters_min[d] = min;
      }
      else
      {
        min = -1; // Goes past the edge
        counters_min[d] = 0;
      }

      // If the minimum is bigger than the number of blocks in that dimension, then the bin is off completely in
      //  that dimension. There is nothing to integrate.
      if (min >= static_cast<int>(split[d]))
        return;
      index_min[d] = min;

      // The max index in this dimension (we round UP, but when we iterate we'll NOT include this edge)
      if (bin.m_max[d] < extents[d].max)
      {
        max = size_t(ceil((bin.m_max[d] - extents[d].min) / boxSize[d]));
        counters_max[d] = max; // This is where the counter ends
      }
      else
      {
        max = split[d]+1; // Goes past THAT edge
        counters_max[d] = split[d]; // but we don't want to in the counters
      }

      // If the max value is before the min, that means NOTHING is in the bin, and we can return
      if ((max <= min) || (max < 0))
        return;
      index_max[d] = max;

      //std::cout << d << " from " <<  index_min[d] << " to " << index_max[d] << "exc" << std::endl;
    }

    // If you reach here, than at least some of bin is overlapping this box
    size_t counters[nd];
    for (size_t d=0; d<nd; d++)
      counters[d] = counters_min[d];

    bool allDone = false;
    while (!allDone)
    {
      size_t index = getLinearIndex(counters);
      //std::cout << index << ": " << counters[0] << ", " << counters[1] << std::endl;

      //TODO: Find if the box is COMPLETELY held in the bin.

      // Perform the binning
      boxes[index]->centerpointBin(bin);

      size_t d = 0;
      while (d<nd)
      {
        counters[d]++;
        if (counters[d] >= counters_max[d])
        {
          // Roll this counter back to 0 (or whatever the min is)
          counters[d] = counters_min[d];
          // Go up one in a higher dimension
          d++;
          // Reached the maximum of the last dimension. Time to exit the entire loop.
          if (d == nd)
            allDone = true;
        }
        else
          break;
      }
    }

  }


}//namespace MDEvents

}//namespace Mantid

