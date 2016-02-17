#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/System.h"
#include "MantidAPI/BoxController.h"
//#include "MantidDataObjects/BoxCtrlChangesList.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/IMDIterator.h"

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
class DLLExport MDEventWorkspace : public API::IMDEventWorkspace {

public:
  /// Typedef for a shared pointer of this kind of event workspace
  typedef boost::shared_ptr<MDEventWorkspace<MDE, nd>> sptr;
  /// Typedef to access the MDEventType.
  typedef MDE MDEventType;

  MDEventWorkspace(Mantid::API::MDNormalization preferredNormalization =
                       Mantid::API::MDNormalization::VolumeNormalization,
                   Mantid::API::MDNormalization preferredNormalizationHisto =
                       Mantid::API::MDNormalization::VolumeNormalization);

  ~MDEventWorkspace() override;

  /// Returns a clone of the workspace
  std::unique_ptr<MDEventWorkspace> clone() const {
    return std::unique_ptr<MDEventWorkspace>(doClone());
  }

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
  std::vector<Mantid::API::IMDIterator *> createIterators(
      size_t suggestedNumCores = 1,
      Mantid::Geometry::MDImplicitFunction *function = nullptr) const override;

  /// Returns the (normalized) signal at a given coordinates
  signal_t getSignalAtCoord(
      const coord_t *coords,
      const Mantid::API::MDNormalization &normalization) const override;

  /// Returns the (normalized) signal at a given coordinates
  // or 0 if masked
  signal_t getSignalWithMaskAtCoord(
      const coord_t *coords,
      const Mantid::API::MDNormalization &normalization) const override;

  bool isInBounds(const coord_t *coords) const;

  signal_t
  getNormalizedSignal(const API::IMDNode *box,
                      const Mantid::API::MDNormalization &normalization) const;

  void getLinePlot(const Mantid::Kernel::VMD &start,
                   const Mantid::Kernel::VMD &end,
                   API::MDNormalization normalize, std::vector<coord_t> &x,
                   std::vector<signal_t> &y,
                   std::vector<signal_t> &e) const override;

  //------------------------ (END) IMDWorkspace Methods
  //-----------------------------------------

  /** @returns the number of bytes of memory used by the workspace. */
  size_t getMemorySize() const override;

  //------------------------ IMDEventWorkspace Methods
  //-----------------------------------------

  /// Returns the BoxController used in this workspace
  Mantid::API::BoxController_sptr getBoxController() override {
    return m_BoxController;
  }

  /// Returns the BoxController used in this workspace
  Mantid::API::BoxController_const_sptr getBoxController() const override {
    return m_BoxController;
  }

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

  Mantid::API::ITableWorkspace_sptr makeBoxTable(size_t start,
                                                 size_t num) override;
  //------------------------ (END) IMDEventWorkspace Methods
  //-----------------------------------------

  void getBoxes(std::vector<API::IMDNode *> &boxes, size_t maxDepth,
                bool leafOnly) override {
    this->getBox()->getBoxes(boxes, maxDepth, leafOnly);
  }

  void addEvent(const MDE &event);

  size_t addEvents(const std::vector<MDE> &events);

  std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>>
  getMinimumExtents(size_t depth = 2) override;

  /// Return true if the underlying box is a MDGridBox.
  bool isGridBox() {
    return dynamic_cast<MDGridBox<MDE, nd> *>(data) != nullptr;
  }

  /** @returns a pointer to the box (MDBox or MDGridBox) contained within, */
  MDBoxBase<MDE, nd> *getBox() { return data; }

  /** @returns a pointer to the box (MDBox or MDGridBox) contained within, const
   * version.  */
  const MDBoxBase<MDE, nd> *getBox() const { return data; }

  /** Set the base-level box contained within.
   * Used in file loading */
  void setBox(API::IMDNode *box) {
    data = dynamic_cast<MDBoxBase<MDE, nd> *>(box);
  }

  /// Apply masking
  void
  setMDMasking(Mantid::Geometry::MDImplicitFunction *maskingRegion) override;

  /// Clear masking
  void clearMDMasking() override;

  /// Get the coordinate system.
  Kernel::SpecialCoordinateSystem getSpecialCoordinateSystem() const override;
  /// Set the coordinate system.
  void setCoordinateSystem(
      const Kernel::SpecialCoordinateSystem coordSystem) override;
  /// make the workspace file backed if it has not been already file backed;
  virtual void setFileBacked(const std::string &fileName);
  /// if workspace was file-backed, this should clear file-backed information
  /// and close back-up files.
  void clearFileBacked(bool LoadFileBackedData) override;

  /// Preferred visual normalizaiton method for any histo workspaces created
  /// from this.
  void setDisplayNormalizationHisto(
      Mantid::API::MDNormalization preferredNormalizationHisto) override;
  Mantid::API::MDNormalization displayNormalizationHisto() const override;

  /// Preferred visual normalization method.
  void setDisplayNormalization(
      Mantid::API::MDNormalization preferredNormalization) override;
  Mantid::API::MDNormalization displayNormalization() const override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  MDEventWorkspace(const MDEventWorkspace<MDE, nd> &other);
  /// Protected copy assignment operator. Assignment not implemented.
  /// Windows Visual Studio 2012 has trouble with declaration without definition
  /// so we provide one that throws an error. This seems template related.
  /// TODO: clean this up.
  MDEventWorkspace<MDE, nd> &operator=(const MDEventWorkspace<MDE, nd> &other) {
    throw std::runtime_error("MDEventWorkspace::operator= not implemented.");
    // this codepath should never be reached, prevent unused parameter warning:
    setTitle(other.getTitle());
    return *this;
  }

  /** MDBox containing all of the events in the workspace. */
  MDBoxBase<MDE, nd> *data;

  /// Box controller in use
  API::BoxController_sptr m_BoxController;
  // boost::shared_ptr<BoxCtrlChangesList > m_BoxController;
  /// Display normalization for the event workspace itself
  Mantid::API::MDNormalization m_displayNormalization;
  /// Display normalization to pass onto generated histo workspaces
  Mantid::API::MDNormalization m_displayNormalizationHisto;

private:
  MDEventWorkspace *doClone() const override {
    return new MDEventWorkspace(*this);
  }

  Kernel::SpecialCoordinateSystem m_coordSystem;
};

} // namespace DataObjects

} // namespace Mantid

#endif /* MDEVENTWORKSPACE_H_ */
