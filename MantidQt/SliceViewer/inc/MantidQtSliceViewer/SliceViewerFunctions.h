#ifndef SLICEVIEWER_FUNCTIONS_H
#define SLICEVIEWER_FUNCTIONS_H

#include "DllOption.h"
#include "MantidKernel/VMD.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDWorkspace.h"

namespace MantidQt {
namespace SliceViewer {

/// Checks if a slice lies within a workspace or not
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER doesSliceCutThroughWorkspace(const Mantid::Kernel::VMD& min, const  Mantid::Kernel::VMD& max,
  const std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions);

/// Checks if rebin mode is in consistent state
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER isRebinInConsistentState(Mantid::API::IMDWorkspace* rebinnedWS, bool useRebinMode);

/// Should perform auto color scaling on load
bool EXPORT_OPT_MANTIDQT_SLICEVIEWER shouldAutoScaleForNewlySetWorkspace(bool isFirstWorkspaceOpen, bool isAutoScalingOnLoad);

}
}
#endif