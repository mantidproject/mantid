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
#include "MantidKernel/System.h"
#include <cmath>
#include <numeric>

namespace Mantid {
namespace DataObjects {

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

  /** 0-based index defining the goniometer settings when the event took place*/
  // attribute right after uint16_t runIndex to avoid padding arising from missaligment
  uint16_t goniometerIndex;

  /** Detector ID of the pixel that measured this event. */
  int32_t detectorId;

public:
  // Enum to flag this templated type as a full md event type.
  enum { is_full_mdevent = true };

  //---------------------------------------------------------------------------------------------
  /** Empty constructor */
  MDEvent()
      : MDLeanEvent<nd>(), runIndex(0), goniometerIndex(0), detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal
   * @param errorSquared :: square of the error on the weight
   * */
  MDEvent(const float signal, const float errorSquared)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(0), goniometerIndex(0),
        detectorId(0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error
   *
   * @param signal :: signal
   * @param errorSquared :: square of the error on the weight
   * */
  MDEvent(const double signal, const double errorSquared)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(0), goniometerIndex(0),
        detectorId(0) {}

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
        goniometerIndex(0), detectorId(detectorId) {}

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
        goniometerIndex(0), detectorId(detectorId) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal, error, runIndex, goniometerIndex, and detectorId
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index defining the goniometer settings
   *when the event took place
   * @param detectorId :: ID of the detector that measured this event.
   */
  MDEvent(const double signal, const double errorSquared,
          const uint16_t runIndex, const uint16_t goniometerIndex,
          const int32_t detectorId)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(runIndex),
        goniometerIndex(goniometerIndex), detectorId(detectorId) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with signal, error, runIndex, goniometerIndex, and detectorId
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index defining the goniometer settings
   *when the event took place
   * @param detectorId :: ID of the detector that measured this event.
   */
  MDEvent(const float signal, const float errorSquared, const uint16_t runIndex,
          const uint16_t goniometerIndex, const int32_t detectorId)
      : MDLeanEvent<nd>(signal, errorSquared), runIndex(runIndex),
        goniometerIndex(goniometerIndex), detectorId(detectorId) {}

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
        goniometerIndex(0), detectorId(0) {}
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
        goniometerIndex(0), detectorId(0) {}
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
        goniometerIndex(0), detectorId(detectorId) {}

  MDEvent(const double signal, const double errorSquared,
          const uint16_t runIndex, const int32_t detectorId,
          const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(runIndex),
        goniometerIndex(0), detectorId(detectorId) {}
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers, and the
   *runIndex, goniometerIndex, and detectorID
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index defining the goniometer settings
   *when the event took place
   * @param detectorId :: ID of the detector that measured this event.
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   * */
  MDEvent(const float signal, const float errorSquared, const uint16_t runIndex,
          const uint16_t goniometerIndex, const int32_t detectorId,
          const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(runIndex),
        goniometerIndex(goniometerIndex), detectorId(detectorId) {}

  MDEvent(const double signal, const double errorSquared,
          const uint16_t runIndex, const uint16_t goniometerIndex,
          const int32_t detectorId, const coord_t *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(runIndex),
        goniometerIndex(goniometerIndex), detectorId(detectorId) {}
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
        goniometerIndex(0), detectorId(detectorId) {}
  //---------------------------------------------------------------------------------------------
  /** Constructor with signal and error and an array of centers, and the
   *runIndex, goniometerIndex, detectorID
   *
   * @param signal :: signal (aka weight)
   * @param errorSquared :: square of the error on the weight
   * @param runIndex :: 0-based index of which run in the containing
   *MDEventWorkspace
   * @param goniometerIndex :: 0-based index defining the goniometer settings
   *when the event took place
   * @param detectorId :: ID of the detector that measured this event.
   * @param centers :: pointer to a nd-sized array of values to set for all
   *coordinates.
   */
  MDEvent(const float signal, const float errorSquared, const uint16_t runIndex,
          const uint16_t goniometerIndex, const int32_t detectorId,
          const double *centers)
      : MDLeanEvent<nd>(signal, errorSquared, centers), runIndex(runIndex),
        goniometerIndex(goniometerIndex), detectorId(detectorId) {}
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
  /** @return the goniometerIndex of this event. */
  uint16_t getGoniometerIndex() const { return goniometerIndex; }

  /** Sets the goniometerIndex of this event
   * @param id :: new goniometerIndex value. */
  void setGoniometerIndex(uint16_t id) { goniometerIndex = id; }

  //---------------------------------------------------------------------------------------------
  /** @returns a string identifying the type of event this is. */
  static std::string getTypeName() { return "MDEvent"; }

  /**
   * Static method used to convert a vector of events into a list of their
   * signal, errorSquared, runIndex, goniometerIndex, detectorId, and
   * nd-Dimensional coordinates. All list items are casted into coord_t type.
   * @param events : vector of events
   * @param data : resulting list (of type std::vector) of data events
   * @param ncols : resulting number of entries per event, here 5+nd
   * @param totalSignal : the sum of all event signal attribute
   * @param totalErrSq : the summ of all event errorSquared attribute
   */
  static inline void eventsToData(const std::vector<MDEvent<nd>> &events,
                                  std::vector<coord_t> &data, size_t &ncols,
                                  double &totalSignal, double &totalErrSq) {
    std::vector<std::string> trait_names = {
        "signal", "errorSquared", "runIndex", "goniometerIndex", "detectorId"};
    ncols = trait_names.size() + nd; // number of data items per event
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
      data[index++] = static_cast<coord_t>(event.runIndex);
      data[index++] = static_cast<coord_t>(event.goniometerIndex);
      data[index++] = static_cast<coord_t>(event.detectorId);
      for (size_t d = 0; d < nd; d++)
        data[index++] = event.center[d]; // add event's coordinates
      // Track the total signal
      totalSignal += signal_t(signal);
      totalErrSq += signal_t(errorSquared);
    }
  }

  /**
   * static method converts a vector of data items into a vector of events.
   * It is assumed the data traits for each event are ordered as follows:
   * 1 signal, 2 errorSquared, 3 runIndex, 4 goniometerIndex, 5 detectorId,
   * 6 to 6+nd-1 nd-Dimensional coordinates.
   *
   * Legacy data do not contain goniometerIndex. Hence, a first attempt is
   * tried assuming the data contains goniometerIndex info, by assessing
   * if the size of the data is divisible by the number of traits (5) plus
   * the event's dimension. If that fails, a retry is done assuming the number
   * of traits is 4.
   *
   * @param data : vector data items, all as type coord_t
   * @param events : resulting vector of events
   * @param reserveMemory : reserve memory for events copying. Set to false if
   * one wants to add new events to the existing one.
   */
  static inline void dataToEvents(const std::vector<coord_t> &data,
                                  std::vector<MDEvent<nd>> &events,
                                  bool reserveMemory = true) {
    std::vector<std::string> trait_names = {
        "signal", "errorSquared", "runIndex", "goniometerIndex", "detectorId"};

    size_t numColumns = trait_names.size() + nd;
    size_t numEvents = data.size() / numColumns;

    // is data.size() not divisible by numColumns?
    if (numEvents * numColumns != data.size()) {
      // retry assuming no goniometerIndex info in `data`
      legacyDataToEvents(data, events, reserveMemory);
      return;
    }

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
      coord_t const *const centers = &(data[ii + trait_names.size()]);

      // Create the event with the traits and the coordinates
      events.emplace_back(static_cast<signal_t>(data[ii]),
                          static_cast<signal_t>(data[ii + 1]),
                          static_cast<uint16_t>(data[ii + 2]),
                          static_cast<uint16_t>(data[ii + 4]),
                          static_cast<int32_t>(data[ii + 3]),
                          centers);
    }
  }

  /**
   * static method converts a vector of data items into a vector of events.
   * It is assumed the data items for each event are ordered as follows:
   * 1 signal, 2 errorSquared, 3 runIndex, 4 detectorId,
   * 5 to 5+nd-1 nd-Dimensional coordinates. When creating the event, a value
   * of zero is assigned to attribute "goniometerIndex".
   * @param data : vector data items, all as type coord_t
   * @param events : resulting vector of events
   * @param reserveMemory : reserve memory for events copying. Set to false if
   * one wants to add new events to the existing one.
   */
  static inline void legacyDataToEvents(const std::vector<coord_t> &data,
                                        std::vector<MDEvent<nd>> &events,
                                        bool reserveMemory = true) {
    std::vector<std::string> trait_names = {"signal", "errorSquared",
                                            "runIndex", "detectorId"};

    size_t numColumns = trait_names.size() + nd;
    size_t numEvents = data.size() / numColumns;

    if (numEvents * numColumns != data.size()) {
      throw(std::invalid_argument("wrong input array of data to convert to "
                                  "events, suspected column data for "
                                  "different dimensions/(type of) events "));
    }

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
      coord_t const *const centers = &(data[ii + trait_names.size()]);

      // Create the event with the traits and the coordinates. Uses the
      // constructor that assigns a value of zero to attribute goniometerIndex
      events.emplace_back(static_cast<signal_t>(data[ii]),
                          static_cast<signal_t>(data[ii + 1]),
                          static_cast<uint16_t>(data[ii + 2]),
                          static_cast<int32_t>(data[ii + 3]), centers);
    }
  }
};

} // namespace DataObjects
} // namespace Mantid
