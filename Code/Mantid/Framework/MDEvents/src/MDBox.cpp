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
  TMDE(MDBox)::MDBox(BoxController_sptr controller)
  {
    if (controller->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = controller;
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
   * @return the number of events that were rejected (because of being out of bounds)
   * */
  TMDE(
  size_t MDBox)::addEvent( const MDE & event)
  {
    this->data.push_back(event);

    // Keep the running total of signal and error
    this->m_signal += event.getSignal();
    this->m_errorSquared += event.getErrorSquared();

    return 0;
  }


  //-----------------------------------------------------------------------------------------------
  /** Add several events
   * @param events :: vector of events to be copied.
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDBox)::addEvents(const std::vector<MDE> & events)
  {
    this->data.insert(this->data.end(), events.begin(), events.end());
    //TODO: Running total of signal/error

    return 0;
  }


  //-----------------------------------------------------------------------------------------------
  /** Return true if the box would need to split into a MDGridBox to handle the new events
   * @param num :: number of events that would be added
   * @return true if the box should split
   */
  TMDE(
  bool MDBox)::willSplit(size_t num) const
  {
    if (!this->m_BoxController)
      return false;
    return this->m_BoxController->willSplit(this->data.size(), num);
  }




}//namespace MDEvents

}//namespace Mantid

