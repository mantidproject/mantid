// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IMDEVENTWORKSPACE_H_
#define IMDEVENTWORKSPACE_H_

#include "MantidAPI/BoxController.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDNode.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/ThreadScheduler.h"

namespace Mantid {
namespace API {

/** Abstract base class for multi-dimension event workspaces (MDEventWorkspace).
 * This class will handle as much of the common operations as possible;
 * but since MDEventWorkspace is a templated class, that makes some aspects
 * impossible.
 *
 * @author Janik Zikovsky, SNS
 * @date Dec 3, 2010
 *
 * */
class MANTID_API_DLL IMDEventWorkspace : public API::IMDWorkspace,
                                         public API::MultipleExperimentInfos {
public:
  IMDEventWorkspace();

  /// Returns a clone of the workspace
  IMDEventWorkspace_uptr clone() const {
    return IMDEventWorkspace_uptr(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  IMDEventWorkspace_uptr cloneEmpty() const {
    return IMDEventWorkspace_uptr(doCloneEmpty());
  }

  /// Perform initialization after dimensions (and others) have been set.
  virtual void initialize() = 0;
  IMDEventWorkspace &operator=(const IMDEventWorkspace &) = delete;

  /// Get the minimum extents that hold the data
  virtual std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>>
  getMinimumExtents(size_t depth = 2) const = 0;

  /// Returns some information about the box controller, to be displayed in the
  /// GUI, for example
  virtual std::vector<std::string> getBoxControllerStats() const = 0;

  virtual Mantid::API::BoxController_sptr getBoxController() = 0;
  virtual Mantid::API::BoxController_const_sptr getBoxController() const = 0;

  virtual void getBoxes(std::vector<API::IMDNode *> &boxes, size_t maxDepth,
                        bool leafOnly) = 0;

  /// @return true if the workspace is file-backed
  virtual bool isFileBacked() const = 0;

  /// set filebacked on the contained box
  virtual void setFileBacked() = 0;

  /// Split the top-level MDBox into a MDGridBox.
  virtual void splitBox() = 0;

  /// Refresh the cache (integrated signal of each box)
  virtual void refreshCache() = 0;

  /// Recurse down to a minimum depth
  virtual void setMinRecursionDepth(size_t depth) = 0;

  /// Return the type of event contained, as a string. MDEvent or MDLeanEvent
  virtual std::string getEventTypeName() const = 0;
  /// Return the size(in bytes) for the event, which this workspace contains
  virtual size_t sizeofEvent() const = 0;

  /// Split all boxes that exceed the split threshold.
  virtual void splitAllIfNeeded(Kernel::ThreadScheduler *ts) = 0;

  bool fileNeedsUpdating() const;

  void setFileNeedsUpdating(bool value);

  bool threadSafe() const override;

  virtual void setCoordinateSystem(
      const Mantid::Kernel::SpecialCoordinateSystem coordinateSystem) = 0;

  /// Preferred visual normalizaiton method for any histo workspaces created
  /// from this.
  virtual void setDisplayNormalizationHisto(
      Mantid::API::MDNormalization preferredNormalizationHisto) = 0;
  Mantid::API::MDNormalization displayNormalizationHisto() const override = 0;

  /// Preferred visual normalization method.
  virtual void setDisplayNormalization(
      Mantid::API::MDNormalization preferredNormalization) = 0;
  Mantid::API::MDNormalization displayNormalization() const override = 0;

  // Check if this class has an oriented lattice on a sample object
  virtual bool hasOrientedLattice() const override {
    return MultipleExperimentInfos::hasOrientedLattice();
  }

  virtual void setBox(API::IMDNode *box) = 0;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  IMDEventWorkspace(const IMDEventWorkspace &) = default;

  const std::string toString() const override;
  /// Marker set to true when a file-backed workspace needs its back-end file
  /// updated (by calling SaveMD(UpdateFileBackEnd=1) )
  bool m_fileNeedsUpdating;

private:
  IMDEventWorkspace *doClone() const override = 0;
  IMDEventWorkspace *doCloneEmpty() const override = 0;
};

} // namespace API

} // namespace Mantid

#endif /* IMDEVENTWORKSPACE_H_ */
