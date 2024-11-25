// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxController.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidKernel/ProgressBase.h"

namespace Mantid {
namespace DataObjects {

/** Templated class for the multi-dimensional event workspace.
 *
 * @tparam MDE :: the type of MDEvent in the workspace. This can be, e.g.
 *                MDLeanEvent<nd> or MDEvent<nd>
 * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
 *               an usigned int > 0.
 *               nd must match the number of dimensions in the MDE type!
 *
 *
 * @author Janik Zikovsky, SNS
 * @date Dec 3, 2010
 *
 * */
TMDE_CLASS
class MANTID_DATAOBJECTS_DLL MDEventWorkspace : public API::IMDEventWorkspace {

public:
  /// Typedef for a shared pointer of this kind of event workspace
  using sptr = std::shared_ptr<MDEventWorkspace<MDE, nd>>;
  /// Typedef to access the MDEventType.
  using MDEventType = MDE;

  MDEventWorkspace(
      Mantid::API::MDNormalization preferredNormalization = Mantid::API::MDNormalization::VolumeNormalization,
      Mantid::API::MDNormalization preferredNormalizationHisto = Mantid::API::MDNormalization::VolumeNormalization);
  MDEventWorkspace<MDE, nd> &operator=(const MDEventWorkspace<MDE, nd> &other) = delete;
  virtual ~MDEventWorkspace() override = default;

  /// Returns a clone of the workspace
  std::unique_ptr<MDEventWorkspace> clone() const { return std::unique_ptr<MDEventWorkspace>(doClone()); }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<MDEventWorkspace> cloneEmpty() const { return std::unique_ptr<MDEventWorkspace>(doCloneEmpty()); }

  /// Perform initialization after dimensions (and others) have been set.
  void initialize() override;

  const std::string id() const override;

  //------------------------ IMDWorkspace Methods
  //-----------------------------------------

  /** @returns the number of dimensions in this workspace */
  size_t getNumDims() const override;

  /** @returns the total number of points (events) in this workspace */
  uint64_t getNPoints() const override;
  uint64_t getNEvents() const override { return getNPoints(); }

  /// Creates a new iterator pointing to the first cell (box) in the workspace
  std::vector<std::unique_ptr<Mantid::API::IMDIterator>>
  createIterators(size_t suggestedNumCores = 1,
                  Mantid::Geometry::MDImplicitFunction *function = nullptr) const override;

  /// Returns the (normalized) signal at a given coordinates
  signal_t getSignalAtCoord(const coord_t *coords, const Mantid::API::MDNormalization &normalization) const override;

  /// Returns the (normalized) signal at a given coordinates
  // or 0 if masked
  signal_t getSignalWithMaskAtCoord(const coord_t *coords,
                                    const Mantid::API::MDNormalization &normalization) const override;

  bool isInBounds(const coord_t *coords) const;

  signal_t getNormalizedSignal(const API::IMDNode *box, const Mantid::API::MDNormalization &normalization) const;

  signal_t getNormalizedError(const API::IMDNode *box, const Mantid::API::MDNormalization &normalization) const;

  LinePlot getLinePlot(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
                       API::MDNormalization normalize) const override;

  // Get ordered list of boundaries in position-along-the-line coordinates
  std::set<coord_t> getBoxBoundaryBisectsOnLine(const Kernel::VMD &start, const Kernel::VMD &end, const size_t num_d,
                                                const Kernel::VMD &dir, const coord_t length) const;

  //------------------------ (END) IMDWorkspace Methods
  //-----------------------------------------

  /** @returns the number of bytes of memory used by the workspace. */
  size_t getMemorySize() const override;

  //------------------------ IMDEventWorkspace Methods
  //-----------------------------------------

  /// Returns the BoxController used in this workspace
  Mantid::API::BoxController_sptr getBoxController() override { return m_BoxController; }

  /// Returns the BoxController used in this workspace
  Mantid::API::BoxController_const_sptr getBoxController() const override { return m_BoxController; }

  std::vector<std::string> getBoxControllerStats() const override;

  /// @return true if the workspace is file-backed
  bool isFileBacked() const override { return m_BoxController->isFileBacked(); }

  std::vector<coord_t> estimateResolution() const override;

  void splitAllIfNeeded(Kernel::ThreadScheduler *ts) override;

  void splitTrackedBoxes(Kernel::ThreadScheduler *ts);

  void splitBox() override;

  void refreshCache() override;

  std::string getEventTypeName() const override;
  /// return the size (in bytes) of an event, this workspace contains
  size_t sizeofEvent() const override { return sizeof(MDE); }

  void setMinRecursionDepth(size_t minDepth) override;

  Mantid::API::ITableWorkspace_sptr makeBoxTable(size_t start, size_t num) override;
  //------------------------ (END) IMDEventWorkspace Methods
  //-----------------------------------------

  void getBoxes(std::vector<API::IMDNode *> &boxes, size_t maxDepth, bool leafOnly) override {
    this->getBox()->getBoxes(boxes, maxDepth, leafOnly);
  }

  size_t addEvent(const MDE &event);

  size_t addEvents(const std::vector<MDE> &events);

  std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> getMinimumExtents(size_t depth = 2) const override;

  /// Return true if the underlying box is a MDGridBox.
  bool isGridBox() { return dynamic_cast<MDGridBox<MDE, nd> *>(data.get()) != nullptr; }

  /** @returns a pointer to the box (MDBox or MDGridBox) contained within, */
  MDBoxBase<MDE, nd> *getBox() { return data.get(); }

  /** @returns a pointer to the box (MDBox or MDGridBox) contained within, const
   * version.  */
  const MDBoxBase<MDE, nd> *getBox() const { return data.get(); }

  /** Set the base-level box contained within.
   * Used in file loading */
  void setBox(API::IMDNode *box) override {
    data = std::unique_ptr<MDBoxBase<MDE, nd>>(dynamic_cast<MDBoxBase<MDE, nd> *>(box));
  }

  /// Apply masking
  void setMDMasking(std::unique_ptr<Mantid::Geometry::MDImplicitFunction> maskingRegion) override;

  /// Clear masking
  void clearMDMasking() override;

  /// Get the coordinate system.
  Kernel::SpecialCoordinateSystem getSpecialCoordinateSystem() const override;
  /// Set the coordinate system.
  void setCoordinateSystem(const Kernel::SpecialCoordinateSystem coordSystem) override;
  /// make the workspace file backed if it has not been already file backed;
  virtual void setFileBacked(const std::string &fileName);
  void setFileBacked() override;
  /// if workspace was file-backed, this should clear file-backed information
  /// and close back-up files.
  void clearFileBacked(bool LoadFileBackedData) override;

  /// Preferred visual normalizaiton method for any histo workspaces created
  /// from this.
  void setDisplayNormalizationHisto(Mantid::API::MDNormalization preferredNormalizationHisto) override;
  Mantid::API::MDNormalization displayNormalizationHisto() const override;

  /// Preferred visual normalization method.
  void setDisplayNormalization(Mantid::API::MDNormalization preferredNormalization) override;
  Mantid::API::MDNormalization displayNormalization() const override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  MDEventWorkspace(const MDEventWorkspace<MDE, nd> &other);

  /// Insert box bisects in position-along-line coords in a single dimension
  void getBoundariesInDimension(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &dir,
                                const size_t num_boundaries, const coord_t length, const coord_t dir_current_dim,
                                const coord_t box_size, std::set<coord_t> &mid_points) const;

  /// Box controller in use
  API::BoxController_sptr m_BoxController;

  /** MDBox containing all of the events in the workspace. */
  std::unique_ptr<MDBoxBase<MDE, nd>> data;

  // std::shared_ptr<BoxCtrlChangesList > m_BoxController;
  /// Display normalization for the event workspace itself
  Mantid::API::MDNormalization m_displayNormalization;
  /// Display normalization to pass onto generated histo workspaces
  Mantid::API::MDNormalization m_displayNormalizationHisto;

private:
  MDEventWorkspace *doClone() const override { return new MDEventWorkspace(*this); }

  MDEventWorkspace *doCloneEmpty() const override { return new MDEventWorkspace(); }

  Kernel::SpecialCoordinateSystem m_coordSystem;
};

} // namespace DataObjects

} // namespace Mantid
