// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IEventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/cow_ptr.h"

#include <iosfwd>
#include <vector>

namespace Mantid {
namespace Types {
namespace Core {
class DateAndTime;
}
} // namespace Types
namespace Kernel {
class Unit;
} // namespace Kernel
namespace DataObjects {
class EventWorkspaceMRU;

/// How the event list is sorted.
enum EventSortType {
  UNSORTED,
  TOF_SORT,
  PULSETIME_SORT,
  PULSETIMETOF_SORT,
  PULSETIMETOF_DELTA_SORT,
  TIMEATSAMPLE_SORT
};

//==========================================================================================
/** @class Mantid::DataObjects::EventList

    A class for holding :
      - a list of neutron detection events (TofEvent or WeightedEvent).
      - a list of associated detector ID's.

    This class can switch from holding regular TofEvent's (implied weight of
   1.0)
    or WeightedEvent (where each neutron can have a non-1 weight).
    This is done transparently.

    @author Janik Zikovsky, SNS ORNL
    @date 4/02/2010
*/

class MANTID_DATAOBJECTS_DLL EventList : public Mantid::API::IEventList {
public:
  EventList(const Mantid::API::EventType event_type = Mantid::API::EventType::TOF);

  EventList(EventWorkspaceMRU *mru, specnum_t specNo);

  EventList(const EventList &rhs);

  EventList(const std::vector<Types::Event::TofEvent> &events);

  EventList(const std::vector<WeightedEvent> &events);

  EventList(const std::vector<WeightedEventNoTime> &events);

  ~EventList() override;

  void copyDataFrom(const ISpectrum &source) override;

  void createFromHistogram(const ISpectrum *inSpec, bool GenerateZeros, bool GenerateMultipleEvents,
                           int MaxEventsPerBin);

  EventList &operator=(const EventList &);

  EventList &operator+=(const Types::Event::TofEvent &event);

  EventList &operator+=(const std::vector<Types::Event::TofEvent> &more_events);

  EventList &operator+=(const WeightedEvent &event);

  EventList &operator+=(const std::vector<WeightedEvent> &more_events);

  EventList &operator+=(const std::vector<WeightedEventNoTime> &more_events);

  EventList &operator+=(const EventList &more_events);

  EventList &operator-=(const EventList &more_events);

  bool operator==(const EventList &rhs) const;
  bool operator!=(const EventList &rhs) const;
  bool equals(const EventList &rhs, const double tolTof, const double tolWeight, const int64_t tolPulse) const;

  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it
   *faster.
   * NOTE: Only call this on a un-weighted event list!
   *
   * @param event :: TofEvent to add at the end of the list.
   * */
  inline void addEventQuickly(const Types::Event::TofEvent &event) {
    this->events.emplace_back(event);
    this->setSortOrder(UNSORTED);
  }

  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it
   * faster.
   * @param event :: WeightedEvent to add at the end of the list.
   * */
  inline void addEventQuickly(const WeightedEvent &event) {
    this->weightedEvents.emplace_back(event);
    this->setSortOrder(UNSORTED);
  }

  // --------------------------------------------------------------------------
  /** Append an event to the histogram, without clearing the cache, to make it
   * faster.
   * @param event :: WeightedEventNoTime to add at the end of the list.
   * */
  inline void addEventQuickly(const WeightedEventNoTime &event) {
    this->weightedEventsNoTime.emplace_back(event);
    this->setSortOrder(UNSORTED);
  }

  Mantid::API::EventType getEventType() const override;

  void switchTo(Mantid::API::EventType newType) override;

  WeightedEvent getEvent(size_t event_number);

  std::vector<Types::Event::TofEvent> &getEvents();
  const std::vector<Types::Event::TofEvent> &getEvents() const;

  std::vector<WeightedEvent> &getWeightedEvents();
  const std::vector<WeightedEvent> &getWeightedEvents() const;

  std::vector<WeightedEventNoTime> &getWeightedEventsNoTime();
  const std::vector<WeightedEventNoTime> &getWeightedEventsNoTime() const;

  void clear(const bool removeDetIDs = true) override;
  void clearUnused();

  void setMRU(EventWorkspaceMRU *newMRU);

  void clearData() override;

  void reserve(size_t num) override;

  void sort(const EventSortType order) const;

  void setSortOrder(const EventSortType order) const;

  void sortTof() const;

  void sortPulseTime() const;
  void sortPulseTimeTOF() const;
  void sortTimeAtSample(const double &tofFactor, const double &tofShift, bool forceResort = false) const;

  bool isSortedByTof() const override;

  EventSortType getSortType() const;

  // X-vector accessors. These reset the MRU for this spectrum
  void setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) override;
  MantidVec &dataX() override;
  const MantidVec &dataX() const override;
  const MantidVec &readX() const override;
  Kernel::cow_ptr<HistogramData::HistogramX> ptrX() const override;

  MantidVec &dataDx() override;
  const MantidVec &dataDx() const override;
  const MantidVec &readDx() const override;

  /// Deprecated, use mutableY() instead. Disallowed data accessors - can't
  /// modify Y/E on a EventList
  MantidVec &dataY() override { throw std::runtime_error("EventList: non-const access to Y data is not possible."); }
  /// Deprecated, use mutableE() instead. Disallowed data accessors - can't
  /// modify Y/E on a EventList
  MantidVec &dataE() override { throw std::runtime_error("EventList: non-const access to E data is not possible."); }

  // Allowed data accessors - read-only Y/E histogram VIEWS of an event list
  /// Deprecated, use y() instead. Return a read-only Y histogram view of an
  /// event list
  const MantidVec &dataY() const override;
  /// Deprecated, use e() instead. Return a read-only E histogram view of an
  /// event list
  const MantidVec &dataE() const override;

  MantidVec *makeDataY() const;
  MantidVec *makeDataE() const;

  std::size_t getNumberEvents() const override;
  bool empty() const;

  size_t getMemorySize() const override;

  virtual size_t histogram_size() const;

  void compressEvents(double tolerance, EventList *destination);
  void compressEvents(double tolerance, EventList *destination,
                      std::shared_ptr<std::vector<double>> histogram_bin_edges);
  void compressFatEvents(const double tolerance, const Types::Core::DateAndTime &timeStart, const double seconds,
                         EventList *destination);
  // get EventType declaration
  void generateHistogram(const MantidVec &X, MantidVec &Y, MantidVec &E, bool skipError = false) const override;
  void generateHistogram(const double step, const MantidVec &X, MantidVec &Y, MantidVec &E,
                         bool skipError = false) const;
  void generateHistogramPulseTime(const MantidVec &X, MantidVec &Y, MantidVec &E,
                                  bool skipError = false) const override;

  void generateHistogramTimeAtSample(const MantidVec &X, MantidVec &Y, MantidVec &E, const double &tofFactor,
                                     const double &tofOffset, bool skipError = false) const override;

  void integrate(const double minX, const double maxX, const bool entireRange, double &sum, double &error) const;

  double integrate(const double minX, const double maxX, const bool entireRange) const override;

  void convertTof(std::function<double(double)> func, const int sorting = 0) override;

  void convertTof(const double factor, const double offset = 0.) override;

  void scaleTof(const double factor) override;

  void addTof(const double offset) override;

  void addPulsetime(const double seconds) override;

  void addPulsetimes(const std::vector<double> &seconds) override;

  void maskTof(const double tofMin, const double tofMax) override;
  void maskCondition(const std::vector<bool> &mask) override;

  void getTofs(std::vector<double> &tofs) const override;
  double getTofMin() const override;
  double getTofMax() const override;
  Mantid::Types::Core::DateAndTime getPulseTimeMax() const override;
  Mantid::Types::Core::DateAndTime getPulseTimeMin() const override;
  void getPulseTimeMinMax(Mantid::Types::Core::DateAndTime &tMin, Mantid::Types::Core::DateAndTime &tM) const;
  Mantid::Types::Core::DateAndTime getTimeAtSampleMax(const double &tofFactor, const double &tofOffset) const override;
  Mantid::Types::Core::DateAndTime getTimeAtSampleMin(const double &tofFactor, const double &tofOffset) const override;

  std::vector<double> getTofs() const override;

  /// Return the list of event weight  values
  std::vector<double> getWeights() const override;
  /// Return the list of event weight values
  void getWeights(std::vector<double> &weights) const override;

  /// Return the list of event weight  error values
  std::vector<double> getWeightErrors() const override;
  /// Return the list of event weight error values
  void getWeightErrors(std::vector<double> &weightErrors) const override;

  std::vector<Types::Core::DateAndTime> getPulseTimes() const override;

  /// Get the Pulse-time + TOF for each event in this EventList
  std::vector<Types::Core::DateAndTime> getPulseTOFTimes() const;

  /// Get the Pulse-time + time-of-flight of the neutron up to the sample, for each event in this EventList
  std::vector<Types::Core::DateAndTime> getPulseTOFTimesAtSample(const double &factor, const double &shift) const;

  void setTofs(const MantidVec &tofs) override;

  void reverse();

  void filterByPulseTime(Types::Core::DateAndTime start, Types::Core::DateAndTime stop, EventList &output) const;

  void filterByPulseTime(Kernel::TimeROI const *timeRoi, EventList *output) const;

  void filterInPlace(const Kernel::TimeROI *timeRoi);

  /// Initialize the detector ID's and event type of the destination event lists when splitting this list
  void initializePartials(std::map<int, EventList *> partials) const;

  void multiply(const double value, const double error = 0.0) override;
  EventList &operator*=(const double value);

  void multiply(const MantidVec &X, const MantidVec &Y, const MantidVec &E) override;

  void divide(const double value, const double error = 0.0) override;
  EventList &operator/=(const double value);

  void divide(const MantidVec &X, const MantidVec &Y, const MantidVec &E) override;

  void convertUnitsViaTof(Mantid::Kernel::Unit const *fromUnit, Mantid::Kernel::Unit const *toUnit);
  void convertUnitsQuickly(const double &factor, const double &power);

  /// Returns a copy of the Histogram associated with this spectrum.
  HistogramData::Histogram getHistogram() const;
  /// Returns the Histogram associated with this spectrum. Y and E data is
  /// computed from the event list.
  HistogramData::Histogram histogram() const override;
  HistogramData::Counts counts() const override;
  HistogramData::CountVariances countVariances() const override;
  HistogramData::CountStandardDeviations countStandardDeviations() const override;
  HistogramData::Frequencies frequencies() const override;
  HistogramData::FrequencyVariances frequencyVariances() const override;
  HistogramData::FrequencyStandardDeviations frequencyStandardDeviations() const override;
  const HistogramData::HistogramY &y() const override;
  const HistogramData::HistogramE &e() const override;
  Kernel::cow_ptr<HistogramData::HistogramY> sharedY() const override;
  Kernel::cow_ptr<HistogramData::HistogramE> sharedE() const override;

  void generateCountsHistogramPulseTime(const double &xMin, const double &xMax, MantidVec &Y,
                                        const double TofMin = std::numeric_limits<double>::lowest(),
                                        const double TofMax = std::numeric_limits<double>::max()) const;

protected:
  void checkAndSanitizeHistogram(HistogramData::Histogram &histogram) override;
  void checkWorksWithPoints() const override;
  void checkIsYAndEWritable() const override;

private:
  using ISpectrum::copyDataInto;
  void copyDataInto(EventList &sink) const override;
  void copyDataInto(Histogram1D &sink) const override;

  const HistogramData::Histogram &histogramRef() const override { return m_histogram; }
  HistogramData::Histogram &mutableHistogramRef() override;

  /// Histogram object holding the histogram data. Currently only X.
  HistogramData::Histogram m_histogram;

  /// List of TofEvent (no weights).
  mutable std::vector<Types::Event::TofEvent> events;

  /// List of WeightedEvent's
  mutable std::vector<WeightedEvent> weightedEvents;

  /// List of WeightedEvent's
  mutable std::vector<WeightedEventNoTime> weightedEventsNoTime;

  /// What type of event is in our list.
  Mantid::API::EventType eventType;

  /// Last sorting order
  mutable EventSortType order;

  /// MRU lists of the parent EventWorkspace
  mutable EventWorkspaceMRU *mru;

  /// Mutex that is locked while sorting an event list
  mutable std::mutex m_sortMutex;

  template <class T>
  static typename std::vector<T>::const_iterator findFirstPulseEvent(const std::vector<T> &events,
                                                                     const double seek_pulsetime);

  template <class T>
  typename std::vector<T>::const_iterator findFirstTimeAtSampleEvent(const std::vector<T> &events,
                                                                     const double seek_time, const double &tofFactor,
                                                                     const double &tofOffset) const;

  void generateCountsHistogram(const MantidVec &X, MantidVec &Y) const;
  void generateCountsHistogram(const double step, const MantidVec &X, MantidVec &Y) const;

public:
  static boost::optional<size_t> findLinearBin(const MantidVec &X, const double tof, const double divisor,
                                               const double offset, const bool findExact = true);
  static boost::optional<size_t> findLogBin(const MantidVec &X, const double tof, const double divisor,
                                            const double offset, const bool findExact = true);

private:
  static boost::optional<size_t> findExactBin(const MantidVec &X, const double tof, size_t n_bin);

  void generateCountsHistogramPulseTime(const MantidVec &X, MantidVec &Y) const;

  void generateCountsHistogramTimeAtSample(const MantidVec &X, MantidVec &Y, const double &tofFactor,
                                           const double &tofOffset) const;

  void generateErrorsHistogram(const MantidVec &Y, MantidVec &E) const;

  void switchToWeightedEvents();
  void switchToWeightedEventsNoTime();
  // should not be called externally
  void sortPulseTimeTOFDelta(const Types::Core::DateAndTime &start, const double seconds) const;

  // helper functions are all internal to simplify the code
  template <class T1, class T2> static void minusHelper(std::vector<T1> &events, const std::vector<T2> &more_events);
  template <class T>
  static void compressEventsHelper(const std::vector<T> &events, std::vector<WeightedEventNoTime> &out,
                                   double tolerance);

  template <class T>
  static void createWeightedEvents(std::vector<WeightedEventNoTime> &out, const std::vector<T> &weight,
                                   const std::vector<T> &error,
                                   const std::shared_ptr<std::vector<double>> histogram_bin_edges);

  template <class T>
  static void processWeightedEvents(const std::vector<T> &events, std::vector<WeightedEventNoTime> &out,
                                    const std::shared_ptr<std::vector<double>> histogram_bin_edges,
                                    struct FindBin findBin);

  template <class T>
  static void compressFatEventsHelper(const std::vector<T> &events, std::vector<WeightedEvent> &out,
                                      const double tolerance, const Mantid::Types::Core::DateAndTime &timeStart,
                                      const double seconds);

  template <class T>
  static void histogramForWeightsHelper(const std::vector<T> &events, const MantidVec &X, MantidVec &Y, MantidVec &E);
  template <class T>
  static void histogramForWeightsHelper(const std::vector<T> &events, const double step, const MantidVec &X,
                                        MantidVec &Y, MantidVec &E);
  template <class T>
  static void integrateHelper(std::vector<T> &events, const double minX, const double maxX, const bool entireRange,
                              double &sum, double &error);
  template <class T> void convertTofHelper(std::vector<T> &events, const std::function<double(double)> &func);

  template <class T> void convertTofHelper(std::vector<T> &events, const double factor, const double offset);
  template <class T> void addPulsetimeHelper(std::vector<T> &events, const double seconds);
  template <class T> void addPulsetimesHelper(std::vector<T> &events, const std::vector<double> &seconds);
  template <class T> static std::size_t maskTofHelper(std::vector<T> &events, const double tofMin, const double tofMax);
  template <class T> static std::size_t maskConditionHelper(std::vector<T> &events, const std::vector<bool> &mask);

  template <class T> static void getTofsHelper(const std::vector<T> &events, std::vector<double> &tofs);
  template <class T> static void getWeightsHelper(const std::vector<T> &events, std::vector<double> &weights);
  template <class T> static void getWeightErrorsHelper(const std::vector<T> &events, std::vector<double> &weightErrors);

  /// Compute a time (for instance, pulse-time plus TOF) associated to each event in the list
  template <typename UnaryOperation>
  std::vector<Types::Core::DateAndTime> eventTimesCalculator(const UnaryOperation &timesCalc) const;

  template <class T> static void setTofsHelper(std::vector<T> &events, const std::vector<double> &tofs);
  template <class T>
  static void filterByPulseTimeHelper(std::vector<T> &events, Types::Core::DateAndTime start,
                                      Types::Core::DateAndTime stop, std::vector<T> &output);

  template <class T>
  static void filterByTimeROIHelper(std::vector<T> &events, const std::vector<Kernel::TimeInterval> &intervals,
                                    EventList *output);

  template <class T> void filterInPlaceHelper(Kernel::TimeROI const *timeRoi, typename std::vector<T> &events);

  template <class T> static void multiplyHelper(std::vector<T> &events, const double value, const double error = 0.0);
  template <class T>
  static void multiplyHistogramHelper(std::vector<T> &events, const MantidVec &X, const MantidVec &Y,
                                      const MantidVec &E);
  template <class T>
  static void divideHistogramHelper(std::vector<T> &events, const MantidVec &X, const MantidVec &Y, const MantidVec &E);
  template <class T>
  void convertUnitsViaTofHelper(typename std::vector<T> &events, Mantid::Kernel::Unit const *fromUnit,
                                Mantid::Kernel::Unit const *toUnit);
  template <class T>
  void convertUnitsQuicklyHelper(typename std::vector<T> &events, const double &factor, const double &power);
};

// Methods overloaded to get event vectors.
DLLExport void getEventsFrom(EventList &el, std::vector<Types::Event::TofEvent> *&events);
DLLExport void getEventsFrom(const EventList &el, std::vector<Types::Event::TofEvent> const *&events);
DLLExport void getEventsFrom(EventList &el, std::vector<WeightedEvent> *&events);
DLLExport void getEventsFrom(const EventList &el, std::vector<WeightedEvent> const *&events);
DLLExport void getEventsFrom(EventList &el, std::vector<WeightedEventNoTime> *&events);
DLLExport void getEventsFrom(const EventList &el, std::vector<WeightedEventNoTime> const *&events);

} // namespace DataObjects
} // namespace Mantid
