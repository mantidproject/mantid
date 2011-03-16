#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"

#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"

using namespace Mantid;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace MDEvents
{

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

    size_t indices[nd];
    for (size_t d=0; d<nd; d++) indices[d] = 0;
    for (size_t i=0; i<tot; i++)
    {
      // Create the box
      MDBox<MDE,nd> * myBox = new MDBox<MDE,nd>(bc);
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
  }



  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDGridBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
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
//    size_t tot = 0;
//    for (size_t i=0; i<boxes.size(); i++)
//      tot += boxes[i]->getNPoints();
//    return tot;
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
  /** Returns false; this box is already split.
   * @param num :: number of events that would be added
   */
  TMDE(
  bool MDGridBox)::willSplit(size_t num) const
  {
    (void) num; //Ignore compiler warning
    return false;
  }


  //-----------------------------------------------------------------------------------------------
  /** Split a box that is contained in the GridBox, at the given index,
   * into a MDGridBox.
   * @param index :: index into the boxes vector. Warning: No bounds check is made, don't give stupid values!
   */
  TMDE(
  void MDGridBox)::splitContents(size_t index)
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
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a single MDEvent to the grid box. If the boxes
   * contained within are also gridded, this will recursively push the event
   * down to the deepest level.
   *
   * @param event :: reference to a MDEvent to add.
   * @return the number of events that were rejected (because of being out of bounds)
   * */
  TMDE(
  size_t MDGridBox)::addEvent( const MDE & event)
  {
    //TODO: Should bounds checks be removed for speed?

    bool badEvent = false;
    size_t index = 0;
    for (size_t d=0; d<nd; d++)
    {
      CoordType x = event.getCenter(d);
      int i = int((x - extents[d].min) / boxSize[d]);
      if (i < 0 || i >= int(split[d]))
      {
        badEvent=true;
        break;
      }
      // Accumulate the index
      index += (i * splitCumul[d]);
    }
    if (!badEvent)
    {
      // Keep the running total of signal, error, and # of points
      this->m_signal += event.getSignal();
      this->m_errorSquared += event.getErrorSquared();
      ++this->nPoints;

      // Add it to the contained box, and return how many bads where there
      return boxes[index]->addEvent( event );
    }
    else
      return 1;

//    std::vector<MDE> events;
//    events.push_back(event);
//    return this->addEvents(events);
  }



  //-----------------------------------------------------------------------------------------------
  /** Add several events. For the grid box, this one needs to
   * parcel out which box receives which event.
   *
   * @param events :: vector of events to be copied.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDGridBox)::addEvents(const std::vector<MDE> & events)
  {
    size_t numBad = 0;

    if (this->m_BoxController->useTasksForAddingEvents(events.size()))
    {
      //TODO: Use tasks and threadpool.
      ThreadScheduler * ts = new ThreadSchedulerLargestCost();
      //ts->

      // Create the threadpool
      ThreadPool tp(ts);
      // Finish all threads.
      tp.joinAll();

      // Done!
      delete ts;
    }
    else
    {
      //TODO: Does it make sense to collect vectors to add, in the event that it is a HUGE list,
      // instead of calling the single "addEvent" method

      // --- Go event by event and add them ----
      typename std::vector<MDE>::const_iterator it;
      typename std::vector<MDE>::const_iterator it_end = events.end();
      for (it = events.begin(); it != it_end; it++)
      {
        numBad += addEvent(*it);

      }
    }
    return numBad;
  }



}//namespace MDEvents

}//namespace Mantid

