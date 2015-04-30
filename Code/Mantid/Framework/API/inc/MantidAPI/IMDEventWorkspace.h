#ifndef IMDEVENTWORKSPACE_H_
#define IMDEVENTWORKSPACE_H_

#include "MantidAPI/BoxController.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IMDNode.h"
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
  IMDEventWorkspace(const IMDEventWorkspace &other);
  virtual ~IMDEventWorkspace() {}

  /// Perform initialization after dimensions (and others) have been set.
  virtual void initialize() = 0;

  /// Get the minimum extents that hold the data
  virtual std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>>
  getMinimumExtents(size_t depth = 2) = 0;

  /// Returns some information about the box controller, to be displayed in the
  /// GUI, for example
  virtual std::vector<std::string> getBoxControllerStats() const = 0;

  virtual Mantid::API::BoxController_sptr getBoxController() = 0;
  virtual Mantid::API::BoxController_const_sptr getBoxController() const = 0;

  virtual void getBoxes(std::vector<API::IMDNode *> &boxes, size_t maxDepth,
                        bool leafOnly) = 0;

  /// @return true if the workspace is file-backed
  virtual bool isFileBacked() const = 0;

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

  virtual bool threadSafe() const;

  virtual void setCoordinateSystem(
      const Mantid::Kernel::SpecialCoordinateSystem coordinateSystem) = 0;

protected:
  virtual const std::string toString() const;
  /// Marker set to true when a file-backed workspace needs its back-end file
  /// updated (by calling SaveMD(UpdateFileBackEnd=1) )
  bool m_fileNeedsUpdating;
};

/// Shared pointer to a generic IMDEventWorkspace
typedef boost::shared_ptr<IMDEventWorkspace> IMDEventWorkspace_sptr;

/// Shared pointer to a generic const IMDEventWorkspace
typedef boost::shared_ptr<const IMDEventWorkspace> IMDEventWorkspace_const_sptr;

} // namespace MDEvents

} // namespace Mantid

#endif /* IMDEVENTWORKSPACE_H_ */
