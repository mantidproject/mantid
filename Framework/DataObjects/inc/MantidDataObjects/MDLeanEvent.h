// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_MDLEANEVENT_H_
#define MANTID_DATAOBJECTS_MDLEANEVENT_H_

#include "MantidDataObjects/MortonIndex/BitInterleaving.h"
#include "MantidDataObjects/MortonIndex/CoordinateConversion.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace Mantid {
namespace DataObjects {

/** Templated class holding data about a neutron detection event
 * in N-dimensions (for example, Qx, Qy, Qz, E).
 *
 *  Each neutron has a signal (a float, can be != 1) and an error. This
 * is the same principle as the WeightedEvent in EventWorkspace's
 *
 * This class is meant to be as small in memory as possible, since there
 * will be (many) billions of it.
 * No virtual methods! This adds a pointer to a vtable = 8 bytes of memory
 * per event (plus the overhead of vtable lookups for calls)
 *
 * @tparam nd :: the number of dimensions that each MDLeanEvent will be
 *tracking.
 *               an int > 0.
 *
 * @author Janik Zikovsky, SNS
 * @date Dec 3, 2010
 *
 * */

template <size_t nd> class MDLeanEvent;

template <size_t nd> void swap(MDLeanEvent<nd> &first, MDLeanEvent<nd> &second);

/**
 * Structure to mark the classes, which can switch the
 * "physical" meaning of the union used in MDLeanEvent
 * to store coordinates or index. If some class is
 * defines the type EventAccessType = EventAccessor,
 * it can call private retrieve functions throuhg the
 * api described in MDLeanEvent (struct AccessFor).
 * Keeping the event state in the coordinates mode
 * is the concern of EventAccessor itself.
 */
struct EventAccessor {};

template <size_t nd> class DLLExport MDLeanEvent {
public:
  /**
   * Additional index type defenitions
   */
  using IntT = typename morton_index::IndexTypes<nd, coord_t>::IntType;
  using MortonT = typename morton_index::IndexTypes<nd, coord_t>::MortonType;

  template <class Accessor>
  /**
   * Internal structure to avoid the direct exposing of API
   * functions, which change the state of event (switch between
   * union fields)
   */
  struct AccessFor {
    static std::enable_if_t<
        std::is_same<EventAccessor, typename Accessor::EventAccessType>::value>
    convertToCoordinates(MDLeanEvent<nd> &event,
                         const morton_index::MDSpaceBounds<nd> &space) {
      event.convertToCoordinates(space);
    }
    static std::enable_if_t<
        std::is_same<EventAccessor, typename Accessor::EventAccessType>::value>
    convertToIndex(MDLeanEvent<nd> &event,
                   const morton_index::MDSpaceBounds<nd> &space) {
      event.convertToIndex(space);
    }
    static typename std::enable_if<
        std::is_same<EventAccessor, typename Accessor::EventAccessType>::value,
        MortonT>::type
    getIndex(const MDLeanEvent<nd> &event) {
      return event.getIndex();
    }
  };
  template <class Accessor> friend struct AccessFor;

protected:
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
   * Second member is an index that can be utilized for faster
   * creating the box structure
   */
#pragma pack(push, 1)
  union {
    coord_t center[nd];
    MortonT index;
  };
#pragma pack(pop)

private:
  /**
   * Calculate Morton index for center coordinates for
   * given space and ovveride the memory used for
   * storing center with index
   * @param space :: given space
   */
  void convertToIndex(const morton_index::MDSpaceBounds<nd> &space) {
    index = morton_index::coordinatesToIndex<nd, IntT, MortonT>(center, space);
  }

  /**
   * Calculate coordinates of the center of event from it
   * Morton index in known space and oveerride index with
   * coordinates in memory
   * @param space :: known space
   */
  void convertToCoordinates(const morton_index::MDSpaceBounds<nd> &space) {
    auto coords =
        morton_index::indexToCoordinates<nd, IntT, MortonT>(index, space);
    for (unsigned i = 0; i < nd; ++i)
      center[i] = coords[i];
  }

  const MortonT getIndex() const { return index; }

public:
  /* Will be keeping functions inline for (possible?) performance improvements
   */

  // Enum to flag this templated type as NOT a full md event type.
  enum { is_full_mdevent = false };

  //---------------------------------------------------------------------------------------------
  /** Empty constructor */
  MDLeanEvent() : signal(1.0), errorSquared(1.0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * */
  MDLeanEvent(const double signal, const double errorSquared)
      : signal(float(signal)), errorSquared(float(errorSquared)) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * */
  MDLeanEvent(const float signal, const float errorSquared)
      : signal(signal), errorSquared(errorSquared) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDLeanEvent(const float signal, const float errorSquared,
              const coord_t *centers)
      : signal(signal), errorSquared(errorSquared) {
    for (size_t i = 0; i < nd; i++)
      center[i] = centers[i];
  }
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDLeanEvent(const double signal, const double errorSquared,
              const coord_t *centers)
      : signal(float(signal)), errorSquared(float(errorSquared)) {
    for (size_t i = 0; i < nd; i++)
      center[i] = centers[i];
  }

#ifdef COORDT_IS_FLOAT
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDLeanEvent(const float signal, const float errorSquared,
              const double *centers)
      : signal(signal), errorSquared(errorSquared) {
    for (size_t i = 0; i < nd; i++)
      center[i] = static_cast<coord_t>(centers[i]);
  }
#endif
  //---------------------------------------------------------------------------------------------
  /** Copy constructor
   * @param rhs :: mdevent to copy
   * */
  MDLeanEvent(const MDLeanEvent &rhs)
      : signal(rhs.signal), errorSquared(rhs.errorSquared) {
    for (size_t i = 0; i < nd; i++)
      center[i] = rhs.center[i];
  }

  //---------------------------------------------------------------------------------------------
  friend void swap<nd>(MDLeanEvent &first, MDLeanEvent &second);

  MDLeanEvent &operator=(MDLeanEvent other) {
    swap(*this, other);
    return *this;
  }

  /** @return the n-th coordinate axis value.
   * @param n :: index (0-based) of the dimension you want.
   * */
  coord_t getCenter(const size_t n) const { return center[n]; }

  //---------------------------------------------------------------------------------------------
  /** Returns the array of coordinates
   * @return pointer to the fixed-size array.
   * */
  const coord_t *getCenter() const { return center; }

  //---------------------------------------------------------------------------------------------
  /** Returns the array of coordinates, as a pointer to a non-const
   * array.
   * @return pointer to the fixed-size array.
   * */
  coord_t *getCenterNonConst() { return center; }

  //---------------------------------------------------------------------------------------------
  /** Sets the n-th coordinate axis value.
   * @param n :: index (0-based) of the dimension you want to set
   * @param value :: value to set.
   * */
  void setCenter(const size_t n, const coord_t value) { center[n] = value; }

#ifdef COORDT_IS_FLOAT
  //---------------------------------------------------------------------------------------------
  /** Sets the n-th coordinate axis value.
   * @param n :: index (0-based) of the dimension you want to set
   * @param value :: value to set.
   * */
  void setCenter(const size_t n, const double value) {
    center[n] = static_cast<coord_t>(value);
  }
#endif

  //---------------------------------------------------------------------------------------------
  /** Sets all the coordinates.
   *
   * @param centers :: pointer to a nd-sized array of the values to set.
   * */
  void setCoords(const coord_t *centers) {
    for (size_t i = 0; i < nd; i++)
      center[i] = centers[i];
  }

  //---------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in the event.
   * */
  size_t getNumDims() const { return nd; }

  //---------------------------------------------------------------------------------------------
  /** Returns the signal (weight) of this event.
   * */
  float getSignal() const { return signal; }

  //---------------------------------------------------------------------------------------------
  /** Returns the error (squared) of this event.
   * */
  float getErrorSquared() const { return errorSquared; }

  //---------------------------------------------------------------------------------------------
  /** Returns the error (not squared) of this event.
   *
   * Performance note: This calls sqrt(), which is a slow function. Use
   *getErrorSquared() if possible.
   * @return the error (not squared) of this event.
   * */
  float getError() const { return float(sqrt(errorSquared)); }

  //---------------------------------------------------------------------------------------------
  /** Set the signal of the event
   * @param newSignal :: the signal value  */
  void setSignal(const float newSignal) { signal = newSignal; }

  //---------------------------------------------------------------------------------------------
  /** Set the squared error  of the event
   * @param newerrorSquared :: the error squared value  */
  void setErrorSquared(const float newerrorSquared) {
    errorSquared = newerrorSquared;
  }

  //---------------------------------------------------------------------------------------------
  /** @returns a string identifying the type of event this is. */
  static std::string getTypeName() { return "MDLeanEvent"; }

  //---------------------------------------------------------------------------------------------
  /** @return the run index of this event in the containing MDEventWorkspace.
   *          Always 0: this information is not present in a MDLeanEvent. */
  uint16_t getRunIndex() const { return 0; }

  //---------------------------------------------------------------------------------------------
  /** @return the detectorId of this event.
   *           Always 0: this information is not present in a MDLeanEvent. */
  int32_t getDetectorID() const { return 0; }

  /* static method used to convert vector of lean events into vector of their
   coordinates & signal and error
   @param events    -- vector of events
   @return data     -- vector of events coordinates, their signal and error
   casted to coord_t type
   @return ncols    -- the number of colunts  in the data (it is nd+2 here but
   may be different for other data types)
   @return totalSignal -- total signal in the vector of events
   @return totalErr   -- total error corresponting to the vector of events
  */
  static inline void eventsToData(const std::vector<MDLeanEvent<nd>> &events,
                                  std::vector<coord_t> &data, size_t &ncols,
                                  double &totalSignal, double &totalErrSq) {
    ncols = nd + 2;
    size_t nEvents = events.size();
    data.resize(nEvents * ncols);

    totalSignal = 0;
    totalErrSq = 0;

    size_t index(0);
    for (const MDLeanEvent<nd> &event : events) {
      float signal = event.signal;
      float errorSquared = event.errorSquared;
      data[index++] = static_cast<coord_t>(signal);
      data[index++] = static_cast<coord_t>(errorSquared);
      for (size_t d = 0; d < nd; d++)
        data[index++] = event.center[d];
      // Track the total signal
      totalSignal += signal_t(signal);
      totalErrSq += signal_t(errorSquared);
    }
  }

  /* static method used to convert vector of data into vector of lean events
   @return coord    -- vector of events coordinates, their signal and error
   casted to coord_t type
   @param events    -- vector of events
   @param reserveMemory -- reserve memory for events copying. Set to false if
   one wants to add new events to the existing one.
  */
  static inline void dataToEvents(const std::vector<coord_t> &coord,
                                  std::vector<MDLeanEvent<nd>> &events,
                                  bool reserveMemory = true) {
    // Number of columns = number of dimensions + 2 (signal/error)
    size_t numColumns = (nd + 2);
    size_t numEvents = coord.size() / numColumns;
    if (numEvents * numColumns != coord.size())
      throw(std::invalid_argument("wrong input array of data to convert to "
                                  "lean events, suspected column data for "
                                  "different dimensions/(type of) events "));

    if (reserveMemory) // Reserve the amount of space needed. Significant speed
                       // up (~30% thanks to this)
    {
      events.clear();
      events.reserve(numEvents);
    }
    for (size_t i = 0; i < numEvents; i++) {
      // Index into the data array
      size_t ii = i * numColumns;

      // Point directly into the data block for the centers.
      const coord_t *centers = &(coord[ii + 2]);

      // Create the event with signal, error squared, and the centers
      events.emplace_back(static_cast<signal_t>(coord[ii]),
                          static_cast<signal_t>(coord[ii + 1]), centers);
    }
  }
};

template <size_t ND>
void swap(MDLeanEvent<ND> &first, MDLeanEvent<ND> &second) {
  std::swap(first.signal, second.signal);
  std::swap(first.errorSquared, second.errorSquared);
  // Index always of the same size as center
  std::swap(first.index, second.index);
}

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_MDLEANEVENT_H_ */
