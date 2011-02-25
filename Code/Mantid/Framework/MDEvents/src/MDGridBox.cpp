#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param box :: MDBox containing the events to split */
  TMDE(MDGridBox)::MDGridBox(MDBox<MDE, nd> * box) :
    IMDBox<MDE, nd>()
  {
    if (!box)
      throw std::runtime_error("MDGridBox::ctor(): box is NULL.");
    BoxController_sptr sc = box->getSplitController();
    if (!sc)
      throw std::runtime_error("MDGridBox::ctor(): No BoxController specified in box.");

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
      split[d] = sc->splitInto(d);
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
      MDBox<MDE,nd> * myBox = new MDBox<MDE,nd>(sc);
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
    //TODO: Cache the value!
    size_t tot = 0;
    for (size_t i=0; i<boxes.size(); i++)
      tot += boxes[i]->getNPoints();
    return tot;
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
  /** Add a MDEvent to the box.
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDGridBox)::addEvent( const MDE & event)
  {
    //this->data.push_back(event);

    // Keep the running total of signal and error
    this->m_signal += event.getSignal();
    this->m_errorSquared += event.getErrorSquared();
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns false; this box is already split.
   * @param num :: number of events that would be added
   */
  TMDE(
  bool MDGridBox)::willSplit(size_t num) const
  {
    return false;
  }


  //-----------------------------------------------------------------------------------------------
  /** Add several events. For the grid box, this one needs to
   * parcel out which box receives which event.
   *
   * @param events :: vector of events to be copied.
   */
  TMDE(
  void MDGridBox)::addEvents(const std::vector<MDE> & events)
  {
    //TODO: Does it make sense to collect vectors to add, in the event that it is a HUGE list,
    // instead of calling the single "addEvent" method

    //TODO: Use tasks and threadpool.

    // --- Go event by event and add them ----
    typename std::vector<MDE>::const_iterator it;
    typename std::vector<MDE>::const_iterator it_end = events.end();
    for (it = events.begin(); it != it_end; it++)
    {
      bool badEvent = false;
      size_t index = 0;
      for (size_t d=0; d<nd; d++)
      {
        CoordType x = it->getCenter(d);
        int i = (x - extents[d].min) / boxSize[d];
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
        boxes[index]->addEvent( *it );
      }
    }
  }



}//namespace MDEvents

}//namespace Mantid

