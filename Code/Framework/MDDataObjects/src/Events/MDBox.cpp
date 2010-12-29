#include "MDDataObjects/Events/MDBox.h"
#include "MDDataObjects/Events/MDPoint.h"

namespace Mantid
{
namespace MDDataObjects
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  template <size_t nd, size_t nv, typename TE>
  MDBox<nd,nv,TE>::MDBox() :
    signal(0.0), errorSquared(0.0)
  {
  }




  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  template <size_t nd, size_t nv, typename TE>
  size_t MDBox<nd,nv,TE>::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  template <size_t nd, size_t nv, typename TE>
  size_t MDBox<nd,nv,TE>::getNPoints() const
  {
    return data.size();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the points vector contained within.
   */
  template <size_t nd, size_t nv, typename TE>
  std::vector< MDPoint<nd,nv,TE> > & MDBox<nd,nv,TE>::getPoints()
  {
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated signal from all points within.
   */
  template <size_t nd, size_t nv, typename TE>
  double MDBox<nd,nv,TE>::getSignal() const
  {
    return signal;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the integrated error squared from all points within.
   */
  template <size_t nd, size_t nv, typename TE>
  double MDBox<nd,nv,TE>::getErrorSquared() const
  {
    return errorSquared;
  }




  //-----------------------------------------------------------------------------------------------
  /** Add a point MDPoint to the box.
   * @param point :: reference to a MDPoint to add.
   * */
  template <size_t nd, size_t nv, typename TE>
  void MDBox<nd,nv,TE>::addPoint( const MDPoint<nd,nv,TE> & point)
  {
    this->data.push_back(point);

    // Keep the running total of signal and error
    signal += point.getSignal();
    errorSquared += point.getErrorSquared();

    // Adjust the stats object
    size_t numPoints = data.size();
    for (size_t i=0; i<nd; i++)
    {
      CoordType x = point.getCenter(i);
      MDDimensionStats & stats = dimStats[i]; // Slight speed up by using the reference
      // Check extents
      if (x < stats.min) stats.min = x;
      if (x > stats.max) stats.max = x;
      // Adjust the total (to get the mean)
      stats.total += x;
      // Now adjust the variance (for std dev)
      CoordType var = (x - stats.total / numPoints);
      stats.approxVariance += var*var;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Return the stats for the nth dimension.
   * @param dim :: index of the dimension.
   * */
  template <size_t nd, size_t nv, typename TE>
  MDDimensionStats MDBox<nd,nv,TE>::getStats(const size_t dim) const
  {
    if (dim >= nd) throw std::runtime_error("Invalid dimension index specified.");
    return this->dimStats[dim];
  }







  // Here we export a bunch of version of MDBox with various dimension sizes.
  // We need to define one for every possibility.
  template DLLExport class MDBox<1>;
  template DLLExport class MDBox<2>;
  template DLLExport class MDBox<3>;
  template DLLExport class MDBox<4>;


}//namespace MDDataObjects

}//namespace Mantid

