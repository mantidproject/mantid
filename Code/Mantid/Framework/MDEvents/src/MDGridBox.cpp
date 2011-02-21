#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDGridBox)::MDGridBox() :
    IMDBox<MDE, nd>()
  {
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
    return 0;
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
  /** Add several events
   * @param events :: vector of events to be copied.
   */
  TMDE(
  void MDGridBox)::addEvents(const std::vector<MDE> & events)
  {
    //this->data.insert(this->data.end(), events.begin(), events.end());
  }








  template DLLExport class MDGridBox<MDEvent<1>, 1>;
  template DLLExport class MDGridBox<MDEvent<2>, 2>;
  template DLLExport class MDGridBox<MDEvent<3>, 3>;
  template DLLExport class MDGridBox<MDEvent<4>, 4>;
  template DLLExport class MDGridBox<MDEvent<5>, 5>;
  template DLLExport class MDGridBox<MDEvent<6>, 6>;
  template DLLExport class MDGridBox<MDEvent<7>, 7>;
  template DLLExport class MDGridBox<MDEvent<8>, 8>;
  template DLLExport class MDGridBox<MDEvent<9>, 9>;


}//namespace MDEvents

}//namespace Mantid

