// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDDimensionStats.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadScheduler.h"

namespace Mantid {
namespace DataObjects {

#ifndef __INTEL_COMPILER // As of July 13, the packing has no effect for the
                         // Intel compiler and produces a warning
#pragma pack(push, 4)    // Ensure the structure is no larger than it needs to
#endif

//===============================================================================================
/** Templated class for a multi-dimensional event "box".
 *
 * A box is a container of MDLeanEvent's within a certain range of values
 * within the nd dimensions. This range defines a n-dimensional "box"
 * or rectangular prism.
 *
 * This class is a simple list of points with no more internal structure.
 *
 * @tparam nd :: the number of dimensions that each MDLeanEvent will be
 *tracking.
 *                  an int > 0.
 *
 * @author Janik Zikovsky, SNS
 * @date Dec 7, 2010
 *
 * */

TMDE_CLASS
class MANTID_DATAOBJECTS_DLL MDBox : public MDBoxBase<MDE, nd> {
public:
  MDBox(Mantid::API::BoxController_sptr &splitter, const uint32_t depth = 0, const size_t nBoxEvents = UNDEF_SIZET,
        const size_t boxID = UNDEF_SIZET);

  MDBox(Mantid::API::BoxController *const splitter, const uint32_t depth = 0, const size_t nBoxEvents = UNDEF_SIZET,
        const size_t boxID = UNDEF_SIZET);

  MDBox(Mantid::API::BoxController_sptr &splitter, const uint32_t depth,
        const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector,
        const size_t nBoxEvents = UNDEF_SIZET, const size_t boxID = UNDEF_SIZET);
  MDBox(Mantid::API::BoxController *const splitter, const uint32_t depth,
        const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector,
        const size_t nBoxEvents = UNDEF_SIZET, const size_t boxID = UNDEF_SIZET);

  using EventIterator = typename std::vector<MDE>::const_iterator;
  MDBox(Mantid::API::BoxController *const bc, const uint32_t depth,
        const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> &extentsVector, EventIterator begin,
        EventIterator end);

  MDBox(const MDBox<MDE, nd> &other, Mantid::API::BoxController *const otherBC);

  ~MDBox() override;

  // ----------------------------- ISaveable Methods
  // ------------------------------------------------------
  Kernel::ISaveable *getISaveable() override;
  Kernel::ISaveable *getISaveable() const override;
  void setFileBacked(const uint64_t /*fileLocation*/, const size_t /*fileSize*/, const bool /*markSaved*/) override;
  void setFileBacked() override;
  void clearFileBacked(bool loadDiskBackedData) override;
  //-----------------------------------------------------------------------------------------------
  void saveAt(API::IBoxControllerIO *const, uint64_t /*position*/) const override;
  void loadAndAddFrom(API::IBoxControllerIO *const, uint64_t, size_t, std::vector<coord_t> &) override;
  void loadAndAddFrom(API::IBoxControllerIO *const, uint64_t /*position*/, size_t /* Size */) override;
  void reserveMemoryForLoad(uint64_t /* Size */) override;
  /**drop events data from memory but keep averages (and file-backed info) */
  void clearDataFromMemory() override;

  void clear() override;

  uint64_t getNPoints() const override;
  size_t getDataInMemorySize() const override { return data.size(); }
  uint64_t getTotalDataSize() const override { return getNPoints(); }

  size_t getNumDims() const override;
  size_t getNumMDBoxes() const override;

  /// Get the # of children MDBoxBase'es (non-recursive)
  size_t getNumChildren() const override { return 0; }
  // to avoid casting (which need also the number of dimensions) method say if
  // Node is a box. if not, it is gridbox
  bool isBox() const override { return true; }

  /// Return the indexth child MDBoxBase.
  API::IMDNode *getChild(size_t /*index*/) override { throw std::runtime_error("MDBox does not have children."); }

  /// Sets the children from a vector of children
  void setChildren(const std::vector<API::IMDNode *> & /*boxes*/, const size_t /*indexStart*/,
                   const size_t /*indexEnd*/) override {
    throw std::runtime_error("MDBox cannot have children.");
  }

  /// @return true if events were added to the box (using addEvent()) while the
  /// rest of the event list is cached to disk
  bool isDataAdded() const;

  /**Get vector of events to change. Beware, that calling this funtion for
    file-based workspace sets both dataChanged and dataBusy flags
    first forces disk buffer to write the object contents to HDD when disk
    buffer is full and the second one prevents DB
    from clearing object from memory untill the events are released. One HAS TO
    call releaseEvents when finished using data on file-based WS    */
  std::vector<MDE> &getEvents();
  /**Get vector of constant events to use. Beware, that calling this funtion for
     file-based workspace sets dataBusy flag
     This flag prevents DB from clearing object from memory untill the events
     are released.
     One HAS TO call releaseEvents when finished using data on file-based WS to
     allow DB clearing them  */
  const std::vector<MDE> &getConstEvents() const;
  // the same as getConstEvents above,
  const std::vector<MDE> &getEvents() const;
  void releaseEvents();

  std::vector<MDE> *getEventsCopy() override;

  void getEventsData(std::vector<coord_t> &coordTable, size_t &nColumns) const override;
  void setEventsData(const std::vector<coord_t> &coordTable) override;

  size_t addEvent(const MDE &Evnt) override;
  size_t addEventUnsafe(const MDE &Evnt) override;

  // add range of events
  size_t addEvents(const std::vector<MDE> &events) override;
  // unhide MDBoxBase methods
  size_t addEventsUnsafe(const std::vector<MDE> &events) override;

  /*--------------->  EVENTS from event data
   * <-------------------------------------------------------------*/
  void buildAndAddEvent(const signal_t Signal, const signal_t errorSq, const std::vector<coord_t> &point,
                        uint16_t expInfoIndex, uint16_t goniometerIndex, uint32_t detectorId) override;
  void buildAndAddEventUnsafe(const signal_t Signal, const signal_t errorSq, const std::vector<coord_t> &point,
                              uint16_t expInfoIndex, uint16_t goniometernIndex, uint32_t detectorId) override;
  size_t buildAndAddEvents(const std::vector<signal_t> &sigErrSq, const std::vector<coord_t> &Coord,
                           const std::vector<uint16_t> &expInfoIndex, const std::vector<uint16_t> &goniometernIndex,
                           const std::vector<uint32_t> &detectorId) override;

  //---------------------------------------------------------------------------------------------------------------------------------
  void centerpointBin(MDBin<MDE, nd> &bin, bool *fullyContained) const override;
  void generalBin(MDBin<MDE, nd> &bin, Mantid::Geometry::MDImplicitFunction &function) const override;
  void
  splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = nullptr) override { /* Do nothing with a box default. */ }

  //---------------------------------------------------------------------------------------------------------------------------------
  /** Recalculate signal and various averages dependent on signal and the signal
   * coordinates */
  void refreshCache(Kernel::ThreadScheduler * /*ts*/ = nullptr) override;
  void calculateCentroid(coord_t *centroid) const override;
  void calculateCentroid(coord_t *centroid, const int expInfoIndex) const override;
  coord_t *getCentroid() const override;
  void calculateDimensionStats(MDDimensionStats *stats) const;
  void integrateSphere(Mantid::API::CoordTransform &radiusTransform, const coord_t radiusSquared, signal_t &signal,
                       signal_t &errorSquared, const coord_t innerRadiusSquared = 0.0,
                       const bool useOnePercentBackgroundCorrection = true) const override;
  void centroidSphere(Mantid::API::CoordTransform &radiusTransform, const coord_t radiusSquared, coord_t *centroid,
                      signal_t &signal) const override;
  void integrateCylinder(Mantid::API::CoordTransform &radiusTransform, const coord_t radius, const coord_t length,
                         signal_t &signal, signal_t &errorSquared, std::vector<signal_t> &signal_fit) const override;

  //------------------------------------------------------------------------------------------------------------------------------------
  void getBoxes(std::vector<MDBoxBase<MDE, nd> *> &boxes, size_t /*maxDepth*/, bool /*leafOnly*/);
  void getBoxes(std::vector<API::IMDNode *> &boxes, size_t /*maxDepth*/, bool /*leafOnly*/) override;

  void getBoxes(std::vector<MDBoxBase<MDE, nd> *> &boxes, size_t maxDepth, bool leafOnly,
                Mantid::Geometry::MDImplicitFunction *function);
  void getBoxes(std::vector<API::IMDNode *> &boxes, size_t maxDepth, bool leafOnly,
                Mantid::Geometry::MDImplicitFunction *function) override;

  void getBoxes(std::vector<API::IMDNode *> &outBoxes, const std::function<bool(API::IMDNode *)> &cond) final override;
  //------------------------------------------------------------------------------------------------------------------------------------
  void transformDimensions(std::vector<double> &scaling, std::vector<double> &offset) override;
  //------------------------------------------------------------------------------------------------------------------------------------
  /* Getter to determine if masking is applied.
  @return true if masking is applied.   */
  bool getIsMasked() const override { return m_bIsMasked; }
  /// Setter for masking the box
  void mask() override;
  /// Setter for unmasking the box
  void unmask() override;

protected:
  // the pointer to the class, responsible for saving/restoring this class to
  // the hdd
  mutable std::unique_ptr<Kernel::ISaveable> m_Saveable;
  /** Vector of MDEvent's, in no particular order. */
  mutable std::vector<MDE> data;

  /// Flag indicating that masking has been applied.
  bool m_bIsMasked;

private:
  /// private default copy constructor as the only correct constructor is the
  /// one with the boxController;
  MDBox(const MDBox &);
  /// common part of mdBox constructor
  void initMDBox(const size_t nBoxEvents);
  /// member to avoid reallocation
  std::vector<coord_t> m_tableData;

public:
  /// Typedef for a shared pointer to a MDBox
  using sptr = std::shared_ptr<MDBox<MDE, nd>>;

  /// Typedef for a vector of the conatined events
  using vec_t = std::vector<MDE>;
};

#ifndef __INTEL_COMPILER
#pragma pack(pop) // Return to default packing size
#endif

//------------------------------------------------------------------------------------------------------------------------------------------------------------
/* Internal TMP class to simplify adding events to the box for events and lean
 * events using single interface*/
template <typename MDE, size_t nd> struct IF {
public:
  // create generic events from array of events data and add them to the grid
  // box
  static inline void EXEC(std::vector<MDE> &data, const std::vector<signal_t> &sigErrSq,
                          const std::vector<coord_t> &Coord, const std::vector<uint16_t> &expInfoIndex,
                          const std::vector<uint16_t> &goniometerIndex, const std::vector<uint32_t> &detectorId,
                          size_t nEvents) {
    for (size_t i = 0; i < nEvents; i++) {
      data.emplace_back(sigErrSq[2 * i], sigErrSq[2 * i + 1], expInfoIndex[i], goniometerIndex[i], detectorId[i],
                        &Coord[i * nd]);
    }
  }
  // create single generic event from event's data
  static inline MDE BUILD_EVENT(const signal_t Signal, const signal_t Error, const coord_t *Coord,
                                const uint16_t expInfoIndex, const uint16_t goniometerIndex,
                                const uint32_t detectorId) {
    return MDE(Signal, Error, expInfoIndex, goniometerIndex, detectorId, Coord);
  }
};
/* Specialize for the case of LeanEvent */
template <size_t nd> struct IF<MDLeanEvent<nd>, nd> {
public:
  // create lean events from array of events data and add them to the box
  static inline void EXEC(std::vector<MDLeanEvent<nd>> &data, const std::vector<signal_t> &sigErrSq,
                          const std::vector<coord_t> &Coord, const std::vector<uint16_t> & /*expInfoIndex*/,
                          const std::vector<uint16_t> & /*goniometerIndex*/,
                          const std::vector<uint32_t> & /*detectorId*/, size_t nEvents) {
    for (size_t i = 0; i < nEvents; i++) {
      data.emplace_back(sigErrSq[2 * i], sigErrSq[2 * i + 1], &Coord[i * nd]);
    }
  }
  // create single lean event from event's data
  static inline MDLeanEvent<nd> BUILD_EVENT(const signal_t Signal, const signal_t Error, const coord_t *Coord,
                                            const uint16_t /*expInfoIndex*/, const uint16_t /*goniometerIndex*/,
                                            const uint32_t /*detectorId*/) {
    return MDLeanEvent<nd>(Signal, Error, Coord);
  }
};
} // namespace DataObjects

} // namespace Mantid
