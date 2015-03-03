#ifndef MDEVENT_H_
#define MDEVENT_H_

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidAPI/BoxController.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include <cmath>
#include <numeric>

namespace Mantid {
namespace DataObjects {

// To ensure the structure is as small as possible
#pragma pack(push, 2)

/** Templated class holding data about a neutron detection event
 * in N-dimensions (for example, Qx, Qy, Qz, E).
 *
 * This is an extension to MDLeanEvent which adds:
 *   - 16-bit int for the run_index (index into the vector of ExperimentInfo of
 *the workspace)
 *   - 32-bit int for the detector_id.
 *
 * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
 *               an int > 0.
 *
 * @author Janik Zikovsky, SNS
 * @date Dec 3, 2010
 *
 * */
template <size_t nd> class DLLExport MDEvent : public MDLeanEvent<nd> {
protected:
  /** 0-based index of which run this event belongs to.
   * This refers to the particular ExperimentInfo that is stored in the
   * MDEventWorkspace;
   * and this can be used to find the run number, goniometer settings, etc. as
   * needed
   */
  uint16_t runIndex;

  /** Detector ID of the pixel that measured this event. */
  int32_t detectorId;

public:
  // Enum to flag this templated type as a full md event type.
  enum { is_full_mdevent = true };

  //---------------------------------------------------------------------------------------------
  /** Empty constructor */
  MDEvent() : MDLeanEvent<nd>(), runIndex(0), detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal
   * @param errorSquared :: square of the error on the weight
   * */
  MDEvent(const float signal, const float errorSquared)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(0), detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal
   * @param errorSquared :: square of the error on the weight
   * */
  MDEvent(const double signal, const double errorSquared)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(0), detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal, error, runIndex and detectorId
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param detectorId :: ID of the detector that measured this event.
   */
  MDEvent(const double signal, const double errorSquared,
          const uint16_t runIndex, const int32_t detectorId)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(runIndex),
        detectorId(detectorId) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal, error, runIndex and detectorId
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param detectorId :: ID of the detector that measured this event.
   */
  MDEvent(const float signal, const float errorSquared, const uint16_t runIndex,
          const int32_t detectorId)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(runIndex),
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
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(0),
        detectorId(0) {}
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const double signal, const double errorSquared,
          const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(0),
        detectorId(0) {}
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers, and the
   *runIndex and detectorID
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param detectorId :: ID of the detector that measured this event.
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const float signal, const float errorSquared, const uint16_t runIndex,
          const int32_t detectorId, const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(runIndex),
        detectorId(detectorId) {}

  MDEvent(const double signal, const double errorSquared,
          const uint16_t runIndex, const int32_t detectorId,
          const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(runIndex),
        detectorId(detectorId) {}

#ifdef COORDT_IS_FLOAT
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers, and the
   *runIndex and detectorID
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param detectorId :: ID of the detector that measured this event.
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const float signal, const float errorSquared, const uint16_t runIndex,
          const int32_t detectorId, const double *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(runIndex),
        detectorId(detectorId) {}
#endif

  //---------------------------------------------------------------------------------------------
  /** @return the run index of this event in the containing MDEventWorkspace */
  uint16_t getRunIndex() const { return runIndex; }

  /** Sets the runIndex of this event
   * @param index :: new runIndex value. */
  void setRunIndex(uint16_t index) { runIndex = index; }

  //---------------------------------------------------------------------------------------------
  /** @return the detectorId of this event. */
  int32_t getDetectorID() const { return detectorId; }

  /** Sets the detectorId of this event
   * @param id :: new runIndex value. */
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
  static inline void eventsToData(const std::vector<MDEvent<nd>> &events,
                                  std::vector<coord_t> &data, size_t &ncols,
                                  double &totalSignal, double &totalErrSq) {
    ncols = (nd + 4);
    size_t nEvents = events.size();
    data.resize(nEvents * ncols);

    totalSignal = 0;
    totalErrSq = 0;

    size_t index(0);
    typename std::vector<MDEvent<nd>>::const_iterator it = events.begin();
    typename std::vector<MDEvent<nd>>::const_iterator it_end = events.end();
    for (; it != it_end; ++it) {
      const MDEvent<nd> &event = *it;
      float signal = event.signal;
      float errorSquared = event.errorSquared;
      data[index++] = static_cast<coord_t>(signal);
      data[index++] = static_cast<coord_t>(errorSquared);
      // Additional stuff for MDEvent
      data[index++] = static_cast<coord_t>(event.runIndex);
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
  static inline void dataToEvents(const std::vector<coord_t> &data,
                                  std::vector<MDEvent<nd>> &events,
                                  bool reserveMemory = true) {
    // Number of columns = number of dimensions + 4 (signal/error)+detId+runID
    size_t numColumns = (nd + 4);
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
      coord_t const *const centers = &(data[ii + 4]);

      // Create the event with signal, error squared, and the centers
      events.push_back(MDEvent<nd>(signal_t(data[ii]), signal_t(data[ii + 1]),
                                   uint16_t(data[ii + 2]),
                                   int32_t(data[ii + 3]), centers));
    }
  }
};
// Return to normal packing
#pragma pack(pop)

} // namespace DataObjects

} // namespace Mantid

#endif /* MDEVENT_H_ */
