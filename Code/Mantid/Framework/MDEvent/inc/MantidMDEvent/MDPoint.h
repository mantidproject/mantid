#ifndef MDPOINT_H_
#define MDPOINT_H_

#include "MantidKernel/System.h"
#include <numeric>
#include <math.h>

namespace Mantid
{
namespace MDDataObjects
{

  /** Typedef for the data type to use for coordinate axes.
   * This could be a float or a double, depending on requirements.
   * We can change this in order to compare
   * performance/memory/accuracy requirements.
   */
  typedef double CoordType;

  /** Macro TMDP to make declaring template functions
   * faster. Put this macro before function declarations.
   *
   */
  #define TMDP template <size_t nd, size_t nv, typename TE>


  /** Templated class holding data about a signal from neutron(s) at a given point
   * in N-dimensions (for example, Qx, Qy, Qz, E).
   *
   * To be more general, the MDPoint class can hold a number of vertices
   * that define the corners of the volume occupied.
   *
   * If MDPoint represents a single neutron event, in which case the corner
   * vertices are not needed.
   *
   * Each point has a signal (a float, can be != 1.0) and an error. This
   * is the same principle as the WeightedEvent in EventWorkspace's
   *
   * This class is meant to be as small in memory as possible, since there
   * will be (many) billions of it.
   * No virtual methods! This adds a vtable which uses lots of memory.
   *
   * @tparam nd :: the number of dimensions that each MDPoint will be tracking.
   *               an usigned int > 0.
   *
   * @tparam nv :: number of corner vertices per dimension. If only the
   *               center of the point is required, this == 0.
   *               If all corners are needed, this == nd.
   *               Default value == 0; meaning only the center coordinates are used.
   *
   * @tparam TE :: Type for a bit of extra data that can be carried around in each point.
   *               For example, this could be a single uint32 representing a detector ID.
   *               Or, if more complex things are required, this could be a struct with
   *               a few fields (should not be dynamically allocated like a pointer, in general).
   *               Default value == char[0] (occupying no memory).
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  template <size_t nd, size_t nv = 0, typename TE = char>
  class DLLExport MDPoint
  {
  private:
    /** The signal (aka weight) from the point.
     * For a single neutron event, this will be exactly 1.0 unless modified at some point.
     */
    float signal;

    /** The square of the error carried in this point.
     * For events, this will be 1.0 unless modified by arithmetic.
     * N.B. The square is used for more efficient calculations.
     */
    float errorSquared;

    /** The N-dimensional coordinates of the center.
     * A simple fixed-sized array of (floats or doubles).
     */
    CoordType center[nd];

    /** The vertices of each corner of the data point, describing
     * a n-dimensional parallepiped.
     *
     * The vertices are stored as a nv*nd array of coordinate. There are
     * nv vertices, each with nd dimensions.
     */
    CoordType corners[nv*nd];

    /** Template-specified bit of extra data that can be carried around
     * in each point. For example, this could be a single uint32 representing
     * a detector ID.
     * Or, if more complex things are required, this could be a struct with
     * a few fields (should not be dynamically allocated like a pointer, in general).
     */
    TE extra;

  public:
    /* Will be keeping functions inline for (possible?) performance improvements */

    //---------------------------------------------------------------------------------------------
    /** Empty constructor */
    MDPoint() :
      signal(1.0), errorSquared(1.0)
    {
    }


    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * */
    MDPoint(const float signal, const float errorSquared) :
      signal(signal), errorSquared(errorSquared)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error and an array of coords
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * @param centers :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDPoint(const float signal, const float errorSquared, const CoordType * centers) :
      signal(signal), errorSquared(errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = centers[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Copy constructor
     *
     * @param rhs :: MDPoint to copy
     * @param errorSquared :: errorSquared
     * */
    MDPoint(const MDPoint &rhs) :
      signal(rhs.signal), errorSquared(rhs.errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = rhs.center[i];
    }


    //---------------------------------------------------------------------------------------------
    /** Returns the n-th center coordinate axis value.
     * @param n :: index (0-based) of the dimension you want.
     * */
    CoordType getCenter(const size_t n) const
    {
      return center[n];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns a direct pointer to the center coordinates of this point,
     * which is a [nd]-sized array.
     * */
    CoordType * getCenters()
    {
      return &center;
    }



    //---------------------------------------------------------------------------------------------
    /** Returns the n-th corner coordinate axis value, one dimension at a time
     *
     * @param nvert :: index (0-based) of the corner to set
     * @param ndim :: index (0-based) of the dimension you want to set. Must be 0 <= ndim <= nd
     * */
    CoordType getCorner(const size_t nvert, const size_t ndim) const
    {
      return corners[nvert*nd + ndim];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns a pointer to the n-th corner coordinate.
     *
     * @param nvert :: index (0-based) of the corner to set
     * */
    CoordType * getCorner(const size_t nvert)
    {
      return &corners[nvert*nd];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns a pointer to the corner vertices of this point,
     * which is a [nd][nv] array.
     * */
    CoordType * getCorners()
    {
      return &corners;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets the n-th center coordinate axis value.
     * @param n :: index (0-based) of the dimension you want to set
     * @param value :: value to set.
     * */
    void setCenter(const size_t n, const CoordType value)
    {
      center[n] = value;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets all the center coordinates.
     *
     * @param coords :: pointer to a nd-sized array of the values to set.
     * */
    void setCenters(const CoordType * coords)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = coords[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the n-th center coordinate axis value, one dimension at a time
     *
     * @param nvert :: index (0-based) of the corner to set
     * @param ndim :: index (0-based) of the dimension you want to set. Must be 0 <= ndim <= nd
     * @param value :: value to set.
     * */
    CoordType setCorner(const size_t nvert, const size_t ndim, const CoordType value)
    {
      corners[nvert*nd + ndim] = value;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the n-th center coordinate axis value, one vertex at a time
     *
     * @param nvert :: index (0-based) of the corner to set
     * @param coords :: an array, sized (nd), of coordinates to set.
     * */
    CoordType setCorner(const size_t nvert, const CoordType * coords)
    {
      for (size_t i=0; i<nd; i++)
        corners[nvert*nd + i] = coords[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Sets all the center coordinates.
     *
     * @param coords :: pointer to a [nv*nd]-sized array of the values to set.
     * */
    void setCorners(const CoordType * coords)
    {
      for (size_t i=0; i<nv*nd; i++)
        corners[i] = coords[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the number of dimensions in the point (nd template parameter).
     * */
    size_t getNumDims() const
    {
      return nd;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the signal (weight) of this point.
     * */
    float getSignal() const
    {
      return signal;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the error (squared) of this point.
     * */
    float getErrorSquared() const
    {
      return errorSquared;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the error (not squared) of this point.
     * */
    float getError() const
    {
      return sqrt(errorSquared);
    }

    //---------------------------------------------------------------------------------------------
    /** Returns a reference to the extra type.
     * */
    TE & getExtra()
    {
      return extra;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns a const reference to the extra type.
     * NOTE: For friend classes, you may access the .extra field directly. This will be faster.
     *
     * */
    TE & getExtra() const
    {
      return extra;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets the extra data. The extra data is copied into this MDPoint.
     * Note: It may be advantageous to use getExtra() and modify the
     * extra field directly, since that function returns a reference.
     *
     * @param _extra :: reference to an instance of type TE.
     * */
    void setExtra(TE & _extra)
    {
      extra = _extra;
    }

  };



}//namespace MDDataObjects

}//namespace Mantid


#endif /* MDPOINT_H_ */
