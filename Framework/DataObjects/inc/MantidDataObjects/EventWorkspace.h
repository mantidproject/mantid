// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACE_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ 1

#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/System.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

namespace Mantid {
namespace API {
class Progress;
}

namespace DataObjects {
class EventWorkspaceMRU;

/** \class EventWorkspace

    This class is intended to fulfill the design specified in
    <https://github.com/mantidproject/documents/tree/master/Design/Event
   WorkspaceDetailed Design Document.doc>
 */

class DLLExport EventWorkspace : public API::IEventWorkspace {

public:
  // The name of the workspace type.
  const std::string id() const override { return "EventWorkspace"; }

  // Constructor
  EventWorkspace(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned);

  // Destructor
  ~EventWorkspace() override;

  /// Returns a clone of the workspace
  std::unique_ptr<EventWorkspace> clone() const {
    return std::unique_ptr<EventWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<EventWorkspace> cloneEmpty() const {
    return std::unique_ptr<EventWorkspace>(doCloneEmpty());
  }

  // Initialize the pixels
  void init(const std::size_t &, const std::size_t &,
            const std::size_t &) override;
  void init(const HistogramData::Histogram &histogram) override;

  bool threadSafe() const override;

  //------------------------------------------------------------

  // Returns the number of single indexable items in the workspace
  std::size_t size() const override;

  // Get the blocksize, aka the number of bins in the histogram
  std::size_t blocksize() const override;

  size_t getMemorySize() const override;

  // Get the number of histograms. aka the number of pixels or detectors.
  std::size_t getNumberHistograms() const override;

  EventList &getSpectrum(const size_t index) override {
    invalidateCommonBinsFlag();
    return getSpectrumWithoutInvalidation(index);
  }
  const EventList &getSpectrum(const size_t index) const override;

  //------------------------------------------------------------

  double getTofMin() const override;

  double getTofMax() const override;

  Mantid::Types::Core::DateAndTime getPulseTimeMin() const override;
  Mantid::Types::Core::DateAndTime getPulseTimeMax() const override;
  void getPulseTimeMinMax(Mantid::Types::Core::DateAndTime &xmin,
                          Mantid::Types::Core::DateAndTime &xmax) const;

  Mantid::Types::Core::DateAndTime
  getTimeAtSampleMin(double tofOffset = 0) const override;

  Mantid::Types::Core::DateAndTime
  getTimeAtSampleMax(double tofOffset = 0) const override;

  double getEventXMin() const;
  double getEventXMax() const;
  void getEventXMinMax(double &xmin, double &xmax) const;

  MantidVec &dataX(const std::size_t) override;
  MantidVec &dataY(const std::size_t) override;
  MantidVec &dataE(const std::size_t) override;
  MantidVec &dataDx(const std::size_t) override;
  const MantidVec &dataX(const std::size_t) const override;
  const MantidVec &dataY(const std::size_t) const override;
  const MantidVec &dataE(const std::size_t) const override;
  const MantidVec &dataDx(const std::size_t) const override;
  Kernel::cow_ptr<HistogramData::HistogramX>
  refX(const std::size_t) const override;

  /// Generate a new histogram from specified event list at the given index.
  void generateHistogram(const std::size_t index, const MantidVec &X,
                         MantidVec &Y, MantidVec &E,
                         bool skipError = false) const override;

  /// Generate a new histogram from specified event list at the given index.
  void generateHistogramPulseTime(const std::size_t index, const MantidVec &X,
                                  MantidVec &Y, MantidVec &E,
                                  bool skipError = false) const;

  // Set the x-axis data (histogram bins) for all pixels
  void setAllX(const HistogramData::BinEdges &x) override;

  // Update all X values to fit around all events
  void resetAllXToSingleBin() override;

  // The total number of events across all of the spectra.
  std::size_t getNumberEvents() const override;

  // Type of the events
  Mantid::API::EventType getEventType() const override;

  // Change the event type
  void switchEventType(const Mantid::API::EventType type);

  // Returns true always - an EventWorkspace always represents histogramm-able
  // data
  bool isHistogramData() const override;

  std::size_t MRUSize() const;

  void clearMRU() const override;

  EventSortType getSortType() const;

  // Sort all event lists. Uses a parallelized algorithm
  void sortAll(EventSortType sortType, Mantid::API::Progress *prog) const;
  void sortAllOld(EventSortType sortType, Mantid::API::Progress *prog) const;

  void getIntegratedSpectra(std::vector<double> &out, const double minX,
                            const double maxX,
                            const bool entireRange) const override;
  EventWorkspace &operator=(const EventWorkspace &other) = delete;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  EventWorkspace(const EventWorkspace &other);

private:
  EventWorkspace *doClone() const override { return new EventWorkspace(*this); }
  EventWorkspace *doCloneEmpty() const override {
    return new EventWorkspace(storageMode());
  }

  EventList &getSpectrumWithoutInvalidation(const size_t index) override;

  /** A vector that holds the event list for each spectrum; the key is
   * the workspace index, which is not necessarily the pixelid.
   */
  std::vector<std::unique_ptr<EventList>> data;

  /// Container for the MRU lists of the event lists contained.
  mutable std::unique_ptr<EventWorkspaceMRU> mru;
};

/// shared pointer to the EventWorkspace class
using EventWorkspace_sptr = boost::shared_ptr<EventWorkspace>;
/// shared pointer to a const Workspace2D
using EventWorkspace_const_sptr = boost::shared_ptr<const EventWorkspace>;

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ */
