#ifndef MDEVENT_H_
#define MDEVENT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Dimension.h"
#include <numeric>
#include <math.h>

namespace Mantid
{
namespace MDEvents
{


  /** Macro TMDE to make declaring template functions
   * faster. Put this macro before function declarations.
   * Use:
   * TMDE(void ClassName)::methodName()
   * {
   *    // function body here
   * }
   *
   * @tparam MDE :: the MDEvent type; at first, this will always be MDEvent<nd>
   * @tparam nd :: the number of dimensions in the center coords. Passing this
   *               as a template argument should speed up some code.
   */
  #define TMDE(decl) template <typename MDE, size_t nd> decl<MDE, nd>

  /** Macro to make declaring template classes faster.
   * Use:
   * TMDE_CLASS
   * class ClassName : ...
   */
  #define TMDE_CLASS template <typename MDE, size_t nd>



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
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  template<size_t nd>
  class DLLExport MDEvent
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

    /** The N-dimensional coordinates of the center of the event.
     * A simple fixed-sized array of (floats or doubles).
     */
    CoordType center[nd];

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
    /** Constructor with signal and error and an array of centers
     *
     * @param signal :: signal
     * @param errorSquared :: errorSquared
     * @param centers :: pointer to a nd-sized array of values to set for all coordinates.
     * */
    MDEvent(const float signal, const float errorSquared, const CoordType * centers) :
      signal(signal), errorSquared(errorSquared)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = centers[i];
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
        center[i] = rhs.center[i];
    }


    //---------------------------------------------------------------------------------------------
    /** Returns the n-th coordinate axis value.
     * @param n :: index (0-based) of the dimension you want.
     * */
    CoordType getCenter(const size_t n) const
    {
      return center[n];
    }

    //---------------------------------------------------------------------------------------------
    /** Sets the n-th coordinate axis value.
     * @param n :: index (0-based) of the dimension you want to set
     * @param value :: value to set.
     * */
    void setCenter(const size_t n, const CoordType value)
    {
      center[n] = value;
    }

    //---------------------------------------------------------------------------------------------
    /** Sets all the coordinates.
     *
     * @param centers :: pointer to a nd-sized array of the values to set.
     * */
    void setCoords(const CoordType * centers)
    {
      for (size_t i=0; i<nd; i++)
        center[i] = centers[i];
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
     *
     * Performance note: This calls sqrt(), which is a slow function. Use getErrorSquared() if possible.
     * */
    float getError() const
    {
      return sqrt(errorSquared);
    }

  };



}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENT_H_ */
