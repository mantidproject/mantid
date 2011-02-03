#include "MantidMDEvent/MDBox.h"
#include "MantidMDEvent/MDPoint.h"

namespace Mantid
{
namespace MDDataObjects
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDP
  MDBox<nd,nv,TE>::MDBox() :
    signal(0.0), errorSquared(0.0)
  {
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDP
  void MDBox<nd,nv,TE>::clear()
  {
    signal = 0.0;
    errorSquared = 0.0;
    data.clear();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDP
  size_t MDBox<nd,nv,TE>::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDP
  size_t MDBox<nd,nv,TE>::getNPoints() const
  {
    return data.size();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the points vector contained within.
   */
  TMDP
  std::vector< MDPoint<nd,nv,TE> > & MDBox<nd,nv,TE>::getPoints()
  {
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated signal from all points within.
   */
  TMDP
  double MDBox<nd,nv,TE>::getSignal() const
  {
    return signal;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated error squared from all points within.
   */
  TMDP
  double MDBox<nd,nv,TE>::getErrorSquared() const
  {
    return errorSquared;
  }




  //-----------------------------------------------------------------------------------------------
  /** Add a point MDPoint to the box.
   * @param point :: reference to a MDPoint to add.
   * */
  TMDP
  void MDBox<nd,nv,TE>::addPoint( const MDPoint<nd,nv,TE> & point)
  {
    this->data.push_back(point);

    // Keep the running total of signal and error
    signal += point.getSignal();
    errorSquared += point.getErrorSquared();
  }

  // Here we export a bunch of version of MDBox with various dimension sizes.
  // We need to define one for every possibility.
  template DLLExport class MDBox<1,1,char>;
  template DLLExport class MDBox<2,1,char>;
  template DLLExport class MDBox<3,1,char>;
  template DLLExport class MDBox<4,1,char>;


}//namespace MDDataObjects

}//namespace Mantid

