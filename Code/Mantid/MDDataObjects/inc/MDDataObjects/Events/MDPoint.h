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


  /** Templated class holding data about a neutron detection point
   * in N-dimensions (for example, Qx, Qy, Qz, E).
   *
   *   Each neutron has a signal (a float, can be != 1.0) and an error. This
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
  template <size_t nd, size_t nv = 0, typename TE = char[0]>
  DLLExport class MDPoint
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
     * TODO: Define exactly what each dimension represents.
     */
    CoordType corners[nd][nv];

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
    /** Returns the n-th center coordinate axis value.
     * @param n :: index (0-based) of the dimension you want to set
     * @param n2 :: the second index (0-based) of the corner to set
     * */
    CoordType getCorner(const size_t n, const size_t n2) const
    {
      return corners[n][n2];
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
    /** Sets the n-th center coordinate axis value.
     * @param n :: index (0-based) of the dimension you want to set
     * @param n2 :: the second index (0-based) of the corner to set
     * @param value :: value to set.
     * */
    void setCorner(const size_t n, const size_t n2, const CoordType value)
    {
      corners[n][n2] = value;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets all the center coordinates.
     *
     * @param coords :: pointer to a [nd][nv]-sized array of the values to set.
     * */
    void setCorners(const CoordType * coords)
    {
      for (size_t i=0; i<nd*nv; i++)
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


  };



}//namespace MDDataObjects

}//namespace Mantid


#endif /* MDPOINT_H_ */
