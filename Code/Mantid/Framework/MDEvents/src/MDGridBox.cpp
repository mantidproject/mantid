#include "MantidKernel/Task.h"
#include "MantidKernel/Utils.h"
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


/** If defined, then signal caching is performed as events are added. Otherwise,
 * refreshCache() has to be called.
 */
#undef MDEVENTS_MDGRIDBOX_ONGOING_SIGNAL_CACHE

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
      this->extents[d] = box->getExtents(d);
    // Copy the depth
    this->m_depth = box->getDepth();
    // Re-calculate the volume of the box
    this->calcVolume();

    // Do some computation based on how many splits per each dim.
    size_t tot = 1;
    double volume = 1;
    for (size_t d=0; d<nd; d++)
    {
      // Cumulative multiplier, for indexing
      splitCumul[d] = tot;
      // How many is it split?
      split[d] = bc->getSplitInto(d);
      tot *= split[d];
      // Length of the side of a box in this dimension
      boxSize[d] = (this->extents[d].max - this->extents[d].min) / split[d];
      volume *= boxSize[d];
    }

    //Cache the inverse volume
    double inverseVolume = 1.0 / volume;

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
        CoordType min = this->extents[d].min + boxSize[d] * indices[d];
        myBox->setExtents(d, min, min + boxSize[d]);
      }
      myBox->setInverseVolume(inverseVolume); // Set the cached inverse volume
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
    // Copy the cached numbers from the incoming box. This is quick - don't need to refresh cache
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
   *
   * @param ts :: ThreadScheduler pointer to perform the caching
   *  in parallel. If NULL, it will be performed in series.
   */
  TMDE(
  void MDGridBox)::refreshCache(ThreadScheduler * ts)
  {
    // Clear your total
    nPoints = 0;
    this->m_signal = 0;
    this->m_errorSquared = 0;

    typename boxVector_t::iterator it;
    typename boxVector_t::iterator it_end = boxes.end();

    if (!ts)
    {
      //--------- Serial -----------
      for (it = boxes.begin(); it != it_end; it++)
      {
        IMDBox<MDE,nd> * ibox = *it;
        MDGridBox<MDE,nd> * gbox = dynamic_cast<MDGridBox<MDE,nd> *>(ibox);

        // Refresh the cache of the contents. Recursive.
        if (gbox)
          gbox->refreshCache(NULL);

        // Add up what's in there
        nPoints += ibox->getNPoints();
        this->m_signal += ibox->getSignal();
        this->m_errorSquared += ibox->getErrorSquared();
      }
    }
    else
    {
      //---------- Parallel refresh --------------

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
    // Track how many MDBoxes there are in the overall workspace
    this->m_BoxController->trackNumBoxes(box->getDepth());
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
            // Track how many MDBoxes there are in the overall workspace
            this->m_BoxController->trackNumBoxes(box->getDepth());
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
          if (!ts || (this->nPoints < this->m_BoxController->getAddingEvents_eventsPerTask()))
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
      int i = int((x - this->extents[d].min) / boxSize[d]);
      // NOTE: No bounds checking is done (for performance).
      //if (i < 0 || i >= int(split[d])) return;

      // Accumulate the index
      index += (i * splitCumul[d]);
    }

    // Add it to the contained box
    boxes[index]->addEvent(event);

    // Track the total signal
#ifdef MDEVENTS_MDGRIDBOX_ONGOING_SIGNAL_CACHE
    statsMutex.lock();
    this->m_signal += event.getSignal();
    this->m_errorSquared += event.getErrorSquared();
    statsMutex.unlock();
#endif
  }



  //-----------------------------------------------------------------------------------------------
  /** Add several events.
   *
   * @param events :: vector of events to be copied.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  inline size_t MDGridBox)::addEvents(const std::vector<MDE> & events)
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
        if ((x < this->extents[d].min) || (x >= this->extents[d].max))
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
    // For running the nested loop, counters of each dimension. These are bounded by 0..split[d]
    size_t counters_min[nd];
    size_t counters_max[nd];

    for (size_t d=0; d<nd; d++)
    {
      int min,max;

      // The min index in this dimension (we round down - we'll include this edge)
      if (bin.m_min[d] >= this->extents[d].min)
      {
        min = size_t((bin.m_min[d] - this->extents[d].min) / boxSize[d]);
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
      if (bin.m_max[d] < this->extents[d].max)
      {
        max = int(ceil((bin.m_max[d] - this->extents[d].min) / boxSize[d])) - 1;
        counters_max[d] = max+1; // (the counter looping will NOT include counters_max[d])
      }
      else
      {
        max = split[d]; // Goes past THAT edge
        counters_max[d] = max; // (the counter looping will NOT include max)
      }

      // If the max value is before the min, that means NOTHING is in the bin, and we can return
      if ((max < min) || (max < 0))
        return;
      index_max[d] = max;

      //std::cout << d << " from " << std::setw(5) << index_min[d] << " to " << std::setw(5)  << index_max[d] << "inc" << std::endl;
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

      // Find if the box is COMPLETELY held in the bin.
      bool completelyWithin = true;
      for(size_t dim=0; dim<nd; dim++)
        if ((static_cast<int>(counters[dim]) <= index_min[dim]) ||
            (static_cast<int>(counters[dim]) >= index_max[dim]))
        {
          // The index we are at is at the edge of the integrated area (index_min or index_max-1)
          // That means that the bin only PARTIALLY covers this MDBox
          completelyWithin = false;
          break;
        }

      if (completelyWithin)
      {
        // Box is completely in the bin.
        //std::cout << "Box at index " << counters[0] << ", " << counters[1] << " is entirely contained.\n";
        // Use the aggregated signal and error
        bin.m_signal += boxes[index]->getSignal();
        bin.m_errorSquared += boxes[index]->getErrorSquared();
      }
      else
      {
        // Perform the binning
        boxes[index]->centerpointBin(bin);
      }

      // Increment the counter(s) in the nested for loops.
      allDone = Utils::nestedForLoopIncrement(nd, counters, counters_max, counters_min);
    }

  }




//
//  //-----------------------------------------------------------------------------------------------
//  /** Run a MDBox task inside this box */
//  TMDE(
//  void MDGridBox)::runMDBoxTask(MDBoxTask<MDE,nd> * /*task*/ , const bool /*fullyContained*/)
//  {
//    //if (task->stopOnFullyContained()
//
////    // Fully evaluate this MD Box (we assume that it is NOT fully contained).
////    task->evaluateMDGridBox(this, false);
//  }

}//namespace MDEvents

}//namespace Mantid

