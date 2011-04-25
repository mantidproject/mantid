#include "MantidMDEvents/MDSplitBox.h"
#include "MantidKernel/System.h"
#include "MantidKernel/FunctionTask.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor. Build the split box out of the unsplit one.
   * Distribute the events.
   *
   * @param box :: Original MDBox (unsplit) to be split
   */
  TMDE(
  MDSplitBox)::MDSplitBox(MDBox<MDE, nd> * box)
  : IMDBox<MDE, nd>(box) // copy extents, etc, other common values
  {
    // Initialization
    nPoints = box->getNPoints();
    BoxController_sptr bc = this->m_BoxController;
    if (!bc)
      throw std::runtime_error("MDGridBox::ctor(): No BoxController specified in box.");

    // Need to get stats on each dimension based on the events contained.
    MDDimensionStats stats[nd];
    box->calculateDimensionStats(stats);

    // Now we look for the dimension that is the widest spread (highest variance)
    CoordType highestVariance = -1.0;
    size_t widestDimension = 0;
    for (size_t d=0; d<nd; d++)
    {
      CoordType var = stats[d].getApproxVariance();
//      std::cout << "dim " << d << " has variance " << var << std::endl;
      if (var > highestVariance)
      {
        widestDimension = d;
        highestVariance = var;
      }
    }
//    std::cout << "Will split along dimension " << widestDimension << std::endl;
    dimSplit = widestDimension;
    splitPoint = stats[dimSplit].getMean();

    // Make the left/right boxes
    initBoxes(box);

    // Add the events in the original box
    this->addEvents(box->getEvents());
  }


  //----------------------------------------------------------------------------------------------
  /** Manual constructor. Does NOT use the original data or add events.
   * Instead, you manually specify which dimension to split.
   *
   * @param box :: Original MDBox that is used only for its extents.
   * @param _dimSplit :: dimension index to split
   * @param _splitPoint :: left/right split point in that dimension.
   */
  TMDE(
  MDSplitBox)::MDSplitBox(IMDBox<MDE, nd> * box, size_t _dimSplit, CoordType _splitPoint)
  : IMDBox<MDE, nd>(box) // copy extents, etc, other common values
  {
    // Directly use the given split dimensions and values
    dimSplit = _dimSplit;
    splitPoint = _splitPoint;
    // Make the left/right boxes
    initBoxes(box);
    // DONT add events.
  }


  //----------------------------------------------------------------------------------------------
  /** Initialize the left/right boxes using the
   * dimSplit and splitPoint values saved before.
   * Private method used by constructor(s)
   */
  TMDE(
  void MDSplitBox)::initBoxes(IMDBox<MDE, nd> * box)
  {
    // Create the left and right boxes with the right dimensions
    left = new MDBox<MDE,nd>(box->getBoxController(), box->getDepth() + 1 );
    right = new MDBox<MDE,nd>(box->getBoxController(), box->getDepth() + 1);
    for (size_t d=0; d<nd; d++)
    {
      MDDimensionExtents ext = box->getExtents(d);
      if (d == dimSplit)
      {
        // Split this dimension down along splitPoint
        left->setExtents(d, ext.min, splitPoint);
        right->setExtents(d, splitPoint, ext.max);
      }
      else
      {
        // Copy the other dimensions
        left->setExtents(d, ext.min, ext.max);
        right->setExtents(d, ext.min, ext.max);
      }
    }
    // Volumes have changed
    left->calcVolume();
    right->calcVolume();
  }

    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(
  MDSplitBox)::~MDSplitBox()
  {
  }
  

  //-----------------------------------------------------------------------------------------------
  /** Clear all contents */
  TMDE(
  void MDSplitBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    left->clear();
    right->clear();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDSplitBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDSplitBox)::getNPoints() const
  {
    //Use the cached value
    return nPoints;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of un-split MDBoxes in this box (including all children)
   * @return :: the total # of MDBoxes in all children */
  TMDE(
  size_t MDSplitBox)::getNumMDBoxes() const
  {
    size_t total = 0;
    total += left->getNumMDBoxes();
    total += right->getNumMDBoxes();
    return total;
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a single MDEvent to the split box.
   * If the boxes contained within are also split,
   * this will recursively push the event down to the deepest level.
   *
   * Warning! No bounds checking is done (for performance). It must
   * be known that the event is within the bounds of the grid box before adding.
   *
   * Note! nPoints, signal and error must be re-calculated using refreshCache()
   * after all events have been added.
   *
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  inline void MDSplitBox)::addEvent( const MDE & event)
  {
    // Compare to the split point in the given dimension
    if (event.getCenter(dimSplit) < splitPoint)
    {
      // Go to the left box
      left->addEvent(event);
    }
    else
    {
      // Go to the right box
      right->addEvent(event);
    }

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
  inline size_t MDSplitBox)::addEvents(const std::vector<MDE> & events)
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
  size_t MDSplitBox)::addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
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
  /** Split a box that is contained in the GridBox, at the given index,
   * into a MDGridBox.
   *
   * Thread-safe as long as 'index' is different for all threads.
   *
   * @param index :: index (0 is left, 1 is right)
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL for no recursive splitting.
   */
  TMDE(
  void MDSplitBox)::splitContents(size_t index, Mantid::Kernel::ThreadScheduler * ts)
  {
    // You can only split it if it is a MDBox (not MDGridBox).
    MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>((index==0 ? left : right));

    if (!box) return;
    // Track how many MDBoxes there are in the overall workspace
    this->m_BoxController->trackNumBoxes(box->getDepth());
    // Construct the grid box
    MDSplitBox<MDE, nd> * splitBox = new MDSplitBox<MDE, nd>(box);

    // Delete the old ungridded box
    delete (index==0 ? left : right);
    // And now we have a gridded box instead of a boring old regular box.
    (index==0 ? left : right) = splitBox;

    if (ts)
    {
      // Create a task to split the newly create MDSplitBox.
      ts->push(new FunctionTask(boost::bind(&MDSplitBox<MDE,nd>::splitAllIfNeeded, &*splitBox, ts) ) );
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
  void MDSplitBox)::splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * ts)
  {
    for (size_t i=0; i < 2; ++i)
    {
      MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>( (i==0 ? left : right) );
      if (box)
      {
        // Plain MD-Box. Does it need to split?
        if (this->m_BoxController->willSplit(box->getNPoints(), box->getDepth() ))
        {
          // The MDBox needs to split into a grid box.
          if (!ts)
          {
            // ------ Perform split serially (no ThreadPool) ------
            MDSplitBox<MDE, nd> * gridBox = new MDSplitBox<MDE, nd>(box);
            // Track how many MDBoxes there are in the overall workspace
            this->m_BoxController->trackNumBoxes(box->getDepth());
            // Replace in the array
            (i==0 ? left : right) = gridBox;
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
            ts->push(new FunctionTask(boost::bind(&MDSplitBox<MDE,nd>::splitContents, &*this, i, ts) ) );
          }
        }
      }
      else
      {
        // It should be a MDSplitBox
        MDSplitBox<MDE, nd> * splitBox = dynamic_cast<MDSplitBox<MDE, nd>*>( (i==0 ? left : right) );
        if (splitBox)
        {
          // Now recursively check if this old grid box's contents should be split too
          if (!ts || (this->nPoints < this->m_BoxController->getAddingEvents_eventsPerTask()))
            // Go serially if there are only a few points contained (less overhead).
            splitBox->splitAllIfNeeded(ts);
          else
            // Go parallel if this is a big enough gridbox.
            // Task is : gridBox->splitAllIfNeeded(ts);
            ts->push(new FunctionTask(boost::bind(&MDSplitBox<MDE,nd>::splitAllIfNeeded, &*splitBox, ts) ) );
        }
      }
    }
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
  void MDSplitBox)::refreshCache(ThreadScheduler * ts)
  {
    // Clear your total
    nPoints = 0;
    this->m_signal = 0;
    this->m_errorSquared = 0;

    if (!ts)
    {
      //--------- Serial -----------
      // Add up left and right sides
      left->refreshCache();
      nPoints += left->getNPoints();
      this->m_signal += left->getSignal();
      this->m_errorSquared += left->getErrorSquared();

      right->refreshCache();
      nPoints += right->getNPoints();
      this->m_signal += right->getSignal();
      this->m_errorSquared += right->getErrorSquared();
    }
    else
    {
      //---------- Parallel refresh --------------
      throw std::runtime_error("Not implemented");
    }

  }


  //-----------------------------------------------------------------------------------------------
  /** Perform centerpoint binning on the boxes contained.
   * @param bin :: MDBin object giving the rectangular bound in which to integrate.
   */
  TMDE(
  void MDSplitBox)::centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const
  {
//    // Check extents in each dimension
//    for (size_t d=0; d<nd; ++d)
//    {
//      // Completely out of bound in any dimension?
//      if (this->extents[d].max < bin.m_min[d])
//        return;
//      if (this->extents[d].min > bin.m_max[d])
//        return;
//    }
//    left->centerpointBin(bin);
//    right->centerpointBin(bin);

    CoordType bin_xmin = bin.m_min[dimSplit];
    CoordType bin_xmax = bin.m_max[dimSplit];
    // Bin is out of range in the small side
    if (bin_xmax < this->extents[dimSplit].min)
      return;
    // Out of range on the big side
    if (bin_xmin > this->extents[dimSplit].max)
      return;
    bool doLeft = true;
    bool doRight = true;
    if (bin_xmin > splitPoint)
      doLeft = false;
    if (bin_xmax < splitPoint)
      doRight = false;

//    if (doLeft) left->centerpointBin(bin, fullyContained);
//    if (doRight) right->centerpointBin(bin, fullyContained);

    if (doLeft)
    {
      bool * leftFullyContained = fullyContained;
      if ((bin_xmin < this->extents[dimSplit].min) && (bin_xmax >= splitPoint))
      {
        //std::cout << "Box is fully contained on the left in dimension " << dimSplit << std::endl;
        // The left box is fully contained along the split dimension.
        // Need to set the dimension as fully split
        leftFullyContained = new bool[nd];
        // TODO: would memcpy be faster?
        for (size_t d=0; d<nd; d++)
          leftFullyContained[d] = fullyContained[d];
        // Along the split dimension, we are fully contained.
        leftFullyContained[dimSplit] = true;
      }
      left->centerpointBin(bin, leftFullyContained);
      if (leftFullyContained != fullyContained)
      {
        // Get rid of the newly allocated array
        delete [] leftFullyContained;
      }
    }

    if (doRight)
    {
      bool * RightFullyContained = fullyContained;
      if ((bin_xmin < splitPoint) && (bin_xmax >= this->extents[dimSplit].max))
      {
        //std::cout << "Box is fully contained on the right in dimension " << dimSplit << std::endl;
        // The Right box is fully contained along the split dimension.
        // Need to set the dimension as fully split
        RightFullyContained = new bool[nd];
        // TODO: would memcpy be faster?
        for (size_t d=0; d<nd; d++)
          RightFullyContained[d] = fullyContained[d];
        // Along the split dimension, we are fully contained.
        RightFullyContained[dimSplit] = true;
      }
      right->centerpointBin(bin, RightFullyContained);
      if (RightFullyContained != fullyContained)
      {
        // Get rid of the newly allocated array
        delete [] RightFullyContained;
      }
    }

  }

} // namespace Mantid
} // namespace MDEvents

