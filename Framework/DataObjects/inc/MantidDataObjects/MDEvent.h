// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <cmath>
#include <numeric>

namespace Mantid {
namespace DataObjects {

/** Templated class holding data about a neutron detection event
 * in N-dimensions (for example, Qx, Qy, Qz, E).
 *
 * This is an extension to MDLeanEvent which adds:
 *   - 16-bit int for the expInfoIndex (index into the vector of ExperimentInfo of
 *the workspace)
 *   - 16-bit int for the goniometer index (0-based index determines the
 *     goniometer settings when the event occurred)
 *   - 32-bit int for the detector_id.
 *
 * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
 *               an int > 0.
 *
 * @author Janik Zikovsky, SNS
 * @date Dec 3, 2010
 *
 * */
// Apply visibility attribute for non-MSVC builds (needed for clang/OSX).
#if defined(_MSC_VER)
template <size_t nd> class MDEvent : public MDLeanEvent<nd> {
#else
template <size_t nd> class MANTID_DATAOBJECTS_DLL MDEvent : public MDLeanEvent<nd> {
#endif
protected:
  /** 0-based index of which run this event belongs to.
   * This refers to the particular ExperimentInfo that is stored in the
   * MDEventWorkspace
   */
  uint16_t expInfoIndex;

  /// 0-based index determines the goniometer settings when this event occurred
  uint16_t goniometerIndex;

  /** Detector ID of the pixel that measured this event. */
  int32_t detectorId;

public:
  // Enum to flag this templated type as a full md event type.
  enum { is_full_mdevent = true };

  //---------------------------------------------------------------------------------------------
  /** Empty constructor */
  MDEvent() : MDLeanEvent<nd>(), expInfoIndex(0), goniometerIndex(0), detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal
   * @param errorSquared :: square of the error on the weight
   * */
  MDEvent(const float signal, const float errorSquared)
      : MDLeanEvent<nd>(signal, errorSquared), expInfoIndex(0), goniometerIndex(0), detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal
   * @param errorSquared :: square of the error on the weight
   * */
  MDEvent(const double signal, const double errorSquared)
      : MDLeanEvent<nd>(signal, errorSquared), expInfoIndex(0), goniometerIndex(0), detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal, error, expInfoIndex and detectorId
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param expInfoIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index determines the goniometer
   * settings when this event occurred
   * @param detectorId :: ID of the detector that measured this event.
   */
  MDEvent(const double signal, const double errorSquared, const uint16_t expInfoIndex, const uint16_t goniometerIndex,
          const int32_t detectorId)
      : MDLeanEvent<nd>(signal, errorSquared), expInfoIndex(expInfoIndex), goniometerIndex(goniometerIndex),
        detectorId(detectorId) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal, error, expInfoIndex and detectorId
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param expInfoIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index determines the goniometer
   * settings when this event occurred
   * @param detectorId :: ID of the detector that measured this event.
   */
  MDEvent(const float signal, const float errorSquared, const uint16_t expInfoIndex, const uint16_t goniometerIndex,
          const int32_t detectorId)
      : MDLeanEvent<nd>(signal, errorSquared), expInfoIndex(expInfoIndex), goniometerIndex(goniometerIndex),
        detectorId(detectorId) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const float signal, const float errorSquared, const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), expInfoIndex(0), goniometerIndex(0), detectorId(0) {}
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const double signal, const double errorSquared, const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), expInfoIndex(0), goniometerIndex(0), detectorId(0) {}
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers, and the
   *expInfoIndex and detectorID
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param expInfoIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index determines the goniometer
   * settings when this event occurred
   * @param detectorId :: ID of the detector that measured this event.
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const float signal, const float errorSquared, const uint16_t expInfoIndex, const uint16_t goniometerIndex,
          const int32_t detectorId, const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), expInfoIndex(expInfoIndex), goniometerIndex(goniometerIndex),
        detectorId(detectorId) {}

  MDEvent(const double signal, const double errorSquared, const uint16_t expInfoIndex, const uint16_t goniometerIndex,
          const int32_t detectorId, const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), expInfoIndex(expInfoIndex), goniometerIndex(goniometerIndex),
        detectorId(detectorId) {}

#ifdef COORDT_IS_FLOAT
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers, and the
   *expInfoIndex and detectorID
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param expInfoIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index determines the goniometer
   * settings when this event occurred
   * @param detectorId :: ID of the detector that measured this event.
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const float signal, const float errorSquared, const uint16_t expInfoIndex, const uint16_t goniometerIndex,
          const int32_t detectorId, const double *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), expInfoIndex(expInfoIndex), goniometerIndex(goniometerIndex),
        detectorId(detectorId) {}
#endif

  //---------------------------------------------------------------------------------------------
  /** @return the associated experiment-info index of this event in the containing MDEventWorkspace */
  uint16_t getExpInfoIndex() const { return expInfoIndex; }

  /** Sets the expInfoIndex of this event
   * @param index :: new expInfoIndex value. */
  void setExpInfoIndex(uint16_t index) { expInfoIndex = index; }

  //---------------------------------------------------------------------------------------------
  /** @return the goniometer index of this event*/
  uint16_t getGoniometerIndex() const { return goniometerIndex; }

  /** Sets the expInfoIndex of this event
   * @param index :: new goniometerIndex value. */
  void setGoniometerIndex(uint16_t index) { goniometerIndex = index; }

  //---------------------------------------------------------------------------------------------
  /** @return the detectorId of this event. */
  int32_t getDetectorID() const { return detectorId; }

  /** Sets the detectorId of this event
   * @param id :: new detector ID value. */
  void setDetectorId(int32_t id) { detectorId = id; }

  //---------------------------------------------------------------------------------------------
  /** @returns a string identifying the type of event this is. */
  static std::string getTypeName() { return "MDEvent"; }

  /* static method used to convert vector of lean events into vector of their
   coordinates & signal and error
   @param events    -- vector of events
   @return data     -- vector of events data, namely, their signal and error
   casted to coord_t type
   @return ncols    -- the number of colunts  in the data (it is nd+4 here but
   may be different for other data types)
   @return totalSignal -- total signal in the vector of events
   @return totalErr   -- total error corresponting to the vector of events
  */
  static inline void eventsToData(const std::vector<MDEvent<nd>> &events, std::vector<coord_t> &data, size_t &ncols,
                                  double &totalSignal, double &totalErrSq) {
    ncols = (nd + 5); // nd+signal+error+run+goniom+detID
    size_t nEvents = events.size();
    data.resize(nEvents * ncols);

    totalSignal = 0;
    totalErrSq = 0;

    size_t index(0);
    for (const auto &event : events) {
      float signal = event.signal;
      float errorSquared = event.errorSquared;
      data[index++] = static_cast<coord_t>(signal);
      data[index++] = static_cast<coord_t>(errorSquared);
      // Additional stuff for MDEvent
      data[index++] = static_cast<coord_t>(event.expInfoIndex);
      data[index++] = static_cast<coord_t>(event.goniometerIndex);
      data[index++] = static_cast<coord_t>(event.detectorId);
      for (size_t d = 0; d < nd; d++)
        data[index++] = event.center[d];
      // Track the total signal
      totalSignal += signal_t(signal);
      totalErrSq += signal_t(errorSquared);
    }
  }
  /* static method used to convert vector of data into vector of lean events
   @return data    -- vector of events coordinates, their signal and error
   casted to coord_t type
   @param events    -- vector of events
   @param reserveMemory -- reserve memory for events copying. Set to false if
   one wants to add new events to the existing one.
  */
  static inline void dataToEvents(const std::vector<coord_t> &data, std::vector<MDEvent<nd>> &events,
                                  bool reserveMemory = true) {
    // Number of columns = number of dimensions + 5
    // (signal/error)+detId+gonID+runID
    size_t numColumns = (nd + 5);
    size_t numEvents = data.size() / numColumns;
    if (numEvents * numColumns != data.size())
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
      coord_t const *const centers = &(data[ii + 5]);

      // Create the event with signal, errorSquared, expInfoIndex,
      // goniometerIndex, detectorID, and the centers, in this order
      events.emplace_back(static_cast<signal_t>(data[ii]), static_cast<signal_t>(data[ii + 1]),
                          static_cast<uint16_t>(data[ii + 2]), static_cast<uint16_t>(data[ii + 3]),
                          static_cast<int32_t>(data[ii + 4]), centers);
    }
  }
};

} // namespace DataObjects
} // namespace Mantid
