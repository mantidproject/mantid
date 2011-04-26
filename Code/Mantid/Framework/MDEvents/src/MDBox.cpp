#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDBox)::MDBox() :
    IMDBox<MDE, nd>()
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** ctor
   * @param controller :: BoxController that controls how boxes split
   */
  TMDE(MDBox)::MDBox(BoxController_sptr controller, const size_t depth)
  {
    if (controller->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = controller;
    this->m_depth = depth;
  }

  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    data.clear();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of un-split MDBoxes in this box (including all children)
   * @return :: 1 always since this is just a MD Box*/
  TMDE(
  size_t MDBox)::getNumMDBoxes() const
  {
    return 1;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDBox)::getNPoints() const
  {
    return data.size();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the events vector contained within.
   */
  TMDE(
  std::vector< MDE > & MDBox)::getEvents()
  {
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    out->insert(out->begin(), data.begin(), data.end());
    return out;
  }



  //-----------------------------------------------------------------------------------------------
  /** Calculate the statistics for each dimension of this MDBox, using
   * all the contained events
   * @param stats :: nd-sized fixed array of MDDimensionStats, reset to 0.0 before!
   */
  TMDE(
  void MDBox)::calculateDimensionStats(MDDimensionStats * stats) const
  {
    typename std::vector<MDE>::const_iterator it_end = data.end();
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; it++)
    {
      const MDE & event = *it;
      for (size_t d=0; d<nd; d++)
      {
        stats[d].addPoint( event.getCenter(d) );
      }
    }
  }



  //-----------------------------------------------------------------------------------------------
  /** Add a MDEvent to the box.
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDE & event)
  {
    dataMutex.lock();
    this->data.push_back(event);

    // Keep the running total of signal and error
    this->m_signal += event.getSignal();
    this->m_errorSquared += event.getErrorSquared();
    dataMutex.unlock();
  }


  //-----------------------------------------------------------------------------------------------
  /** Add several events. No bounds checking is made!
   *
   * @param events :: vector of events to be copied.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDBox)::addEvents(const std::vector<MDE> & events)
  {
    dataMutex.lock();
    // Copy all the events
    this->data.insert(this->data.end(), events.begin(), events.end());

    //Running total of signal/error
    for(typename std::vector<MDE>::const_iterator it = events.begin(); it != events.end(); it++)
    {
      this->m_signal += it->getSignal();
      this->m_errorSquared += it->getErrorSquared();
    }

    dataMutex.unlock();
    return 0;
  }

  //-----------------------------------------------------------------------------------------------
  /** Perform centerpoint binning of events.
   * @param bin :: MDBin object giving the limits of events to accept.
   */
  TMDE(
  void MDBox)::centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const
  {
    if (fullyContained)
    {
      size_t d;
      for (d=0; d<nd; ++d)
      {
        if (!fullyContained[d]) break;
      }
      if (d == nd)
      {
//        std::cout << "MDBox at depth " << this->m_depth << " was fully contained in bin " << bin.m_index << ".\n";
        // All dimensions are fully contained, so just return the cached total signal instead of counting.
        bin.m_signal += this->m_signal;
        bin.m_errorSquared += this->m_errorSquared;
        return;
      }
    }

    typename std::vector<MDE>::const_iterator it = data.begin();
    typename std::vector<MDE>::const_iterator it_end = data.end();

    // For each MDEvent
    for (; it != it_end; ++it)
    {
      // Go through each dimension
      size_t d;
      for (d=0; d<nd; ++d)
      {
        // Check that the value is within the bounds given. (Rotation is for later)
        CoordType x = it->getCenter(d);
        if (x < bin.m_min[d])
          break;
        if (x >= bin.m_max[d])
          break;
      }
      // If the loop reached the end, then it was all within bounds.
      if (d == nd)
      {
        // Accumulate error and signal
        bin.m_signal += it->getSignal();
        bin.m_errorSquared += it->getErrorSquared();
      }
    }
  }

//
//  //-----------------------------------------------------------------------------------------------
//  /** Run a MDBox task inside this box */
//  TMDE(
//  void MDBox)::runMDBoxTask(MDBoxTask<MDE,nd> * task, const bool fullyContained)
//  {
//    // Fully evaluate this MD Box
//    task->evaluateMDBox(this, fullyContained);
//  }

}//namespace MDEvents

}//namespace Mantid

