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
  /** Returns a reference to the points vector contained within.
   */
  TMDE(
  std::vector< MDEvent<nd> > & MDBox)::getPoints()
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
   * @param point :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDEvent<nd> & point)
  {
    this->data.push_back(point);

    // Keep the running total of signal and error
    signal += point.getSignal();
    errorSquared += point.getErrorSquared();
  }

  // Here we export a bunch of version of MDBox with various dimension sizes.
  // We need to define one for every possibility.
  template DLLExport class MDBox<1>;
  template DLLExport class MDBox<2>;
  template DLLExport class MDBox<3>;
  template DLLExport class MDBox<4>;


}//namespace MDEvents

}//namespace Mantid

