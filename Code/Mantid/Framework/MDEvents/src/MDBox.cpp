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
  /** Add several events.
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





}//namespace MDEvents

}//namespace Mantid

