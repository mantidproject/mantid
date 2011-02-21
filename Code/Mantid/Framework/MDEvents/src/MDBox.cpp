#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDBox)::MDBox() :
    signal(0.0), errorSquared(0.0)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(void MDBox)::clear()
  {
    signal = 0.0;
    errorSquared = 0.0;
    data.clear();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(size_t MDBox)::getNumDims() const
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
  /** Returns the integrated signal from all points within.
   */
  TMDE(
  double MDBox)::getSignal() const
  {
    return signal;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated error squared from all points within.
   */
  TMDE(
  double MDBox)::getErrorSquared() const
  {
    return errorSquared;
  }




  //-----------------------------------------------------------------------------------------------
  /** Add a MDEvent to the box.
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDE & event)
  {
    this->data.push_back(event);

    // Keep the running total of signal and error
    signal += event.getSignal();
    errorSquared += event.getErrorSquared();
  }








  template DLLExport class MDBox<MDEvent<1>, 1>;
  template DLLExport class MDBox<MDEvent<2>, 2>;
  template DLLExport class MDBox<MDEvent<3>, 3>;
  template DLLExport class MDBox<MDEvent<4>, 4>;
  template DLLExport class MDBox<MDEvent<5>, 5>;
  template DLLExport class MDBox<MDEvent<6>, 6>;
  template DLLExport class MDBox<MDEvent<7>, 7>;
  template DLLExport class MDBox<MDEvent<8>, 8>;
  template DLLExport class MDBox<MDEvent<9>, 9>;


}//namespace MDEvents

}//namespace Mantid

