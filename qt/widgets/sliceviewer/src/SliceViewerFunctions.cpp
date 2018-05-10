#include "MantidQtWidgets/SliceViewer/SliceViewerFunctions.h"

namespace MantidQt {
namespace SliceViewer {

/**
 * Checks if a slice, defined by a  min and a max vector, takes a slice which
 * cuts through the workspace
 * @param min: the min boundary of the slice
 * @param max: the max boundary of the slice
 * @param dimensions: a vector of dimension objects of the workspace which will
 * be sliced
 * @returns true if the slice goes fully or partially through the workspace
 */
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER doesSliceCutThroughWorkspace(
    const Mantid::Kernel::VMD &min, const Mantid::Kernel::VMD &max,
    const std::vector<Mantid::Geometry::MDHistoDimension_sptr> &dimensions) {
  auto valueBetweenMinMax = [](const Mantid::Kernel::VMD_t value,
                               const Mantid::Kernel::VMD_t min,
                               const Mantid::Kernel::VMD_t max) {
    return value >= min && value <= max;
  };

  int dimCounter = 0;
  auto cutsThroughWorkspace = true;

  // Check in either dimension if the the min and max values are withing the
  // workspace boundaries
  for (const auto &dimension : dimensions) {
    const auto minDimension =
        static_cast<Mantid::Kernel::VMD_t>(dimension->getMinimum());
    const auto maxDimension =
        static_cast<Mantid::Kernel::VMD_t>(dimension->getMaximum());

    // If the the value for min and max is not in the min-max range of the
    // dimension of the workspace
    // then the cut is neither full nor partial
    if (!valueBetweenMinMax(min[dimCounter], minDimension, maxDimension) &&
        !valueBetweenMinMax(max[dimCounter], minDimension, maxDimension)) {
      cutsThroughWorkspace = false;
      break;
    }
    ++dimCounter;
  }
  return cutsThroughWorkspace;
}

/**
 * Checks if the colors scale range should be automatically set. We should
 * provide auto scaling
 * on load if the workspaces is either loaded the first time or if it is
 * explictly selected.
 * @param isFirstWorkspaceOpen: if the workspace is being loaded for the first
 * time
 * @param isAutoScalingOnLoad: is auto scaling on load selected
 * @returns true if autos scaling on load should be performed else false
 */
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER shouldAutoScaleForNewlySetWorkspace(
    bool isFirstWorkspaceOpen, bool isAutoScalingOnLoad) {
  return !isFirstWorkspaceOpen || isAutoScalingOnLoad;
}

/*
 * Checks if the rebinning is in a consistent state, ie if rebin mode is
 * selected and there
 * is a rebin workspace or there is no rebin workspace and no rebin mode
 * selected.
 * The state is inconsistent if rebin mode is selected and there is no
 * workspace.
 * @param rebinnedWS: a pointer to an MD overlay workspace
 * @param useRebinMode: indicates if rebinning is to be used
 * @returns true rebin state is consistent, else false
 */
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER isRebinInConsistentState(
    Mantid::API::IMDWorkspace *rebinnedWS, bool useRebinMode) {
  return rebinnedWS && useRebinMode;
}
} // namespace SliceViewer
} // namespace MantidQt
