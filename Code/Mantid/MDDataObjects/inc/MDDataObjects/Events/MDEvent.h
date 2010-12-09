#ifndef MDEVENT_H_
#define MDEVENT_H_

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



  /** Templated class holding data about a neutron detection event
   * in N-dimensions (for example, Qx, Qy, Qz, E).
   *
   *   Each neutron has a signal (a float, can be != 1) and an error. This
   * is the same principle as the WeightedEvent in EventWorkspace's
   *
   * This class is meant to be as small in memory as possible, since there
   * will be (many) billions of it.
   * No virtual methods! This adds a vtable which uses lots of memory.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *               an int > 0.
   * @tparam ne :: the number of extra (non-float) ints of information to store.
   *               These could be detector ids, run numbers, etc. and will consist
   *               of signed 32-bit ints
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  template <size_t nd>
  DLLExport class MDEvent
  {
  private:
    /** The signal (aka weight) from the neutron event.
     * Will be exactly 1.0 unless modified at some point.
     */
    float signal;

    /** The square of the error carried in this event.
     * Will be 1.0 unless modified by arithmetic.
     * The square is used for more efficient calculations.
     */
    float errorSquared;

    /** The N-dimensional coordinates. A simple
     * fixed-sized array of (floats or doubles).
     */
    CoordType coord[nd];

  public:
    /* Will be keeping functions inline for (possible?) performance improvements */

    //---------------------------------------------------------------------------------------------
    /** Empty constructor */
    MDEvent() :
      signal(1.0), errorSquared(1.0)
    {
    }


    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * */
    MDEvent(const float signal, const float errorSquared) :
      signal(signal), errorSquared(errorSquared)
    {
    }

    //---------------------------------------------------------------------------------------------
    /** Constructor with signal and error and an array of coords
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * @param coords :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDEvent(const float signal, const float errorSquared, const CoordType * coords) :
      signal(signal), errorSquared(errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        coord[i] = coords[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Copy constructor
     *
     * @param rhs :: mdevent to copy
     * @param errorSquared :: errorSquared
     * */
    MDEvent(const MDEvent &rhs) :
      signal(rhs.signal), errorSquared(rhs.errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        coord[i] = rhs.coord[i];
    }


    //---------------------------------------------------------------------------------------------
    /** Returns the n-th coordinate axis value.
     * @param n :: index (0-based) of the dimension you want.
     * */
    CoordType getCoord(const size_t n) const
    {
      return coord[n];
    }

    //---------------------------------------------------------------------------------------------
    /** Sets the n-th coordinate axis value.
     * @param n :: index (0-based) of the dimension you want to set
     * @param value :: value to set.
     * */
    void setCoord(const size_t n, const CoordType value)
    {
      coord[n] = value;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets all the coordinates.
     *
     * @param coords :: pointer to a nd-sized array of the values to set.
     * */
    void setCoords(const CoordType * coords)
    {
      for (size_t i=0; i<nd; i++)
        coord[i] = coords[i];
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the number of dimensions in the event.
     * */
    size_t getNumDims() const
    {
      return nd;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the signal (weight) of this event.
     * */
    float getSignal() const
    {
      return signal;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the error (squared) of this event.
     * */
    float getErrorSquared() const
    {
      return errorSquared;
    }

    //---------------------------------------------------------------------------------------------
    /** Returns the error (not squared) of this event.
     * */
    float getError() const
    {
      return sqrt(errorSquared);
    }

  };



}//namespace MDDataObjects

}//namespace Mantid


#endif /* MDEVENT_H_ */
