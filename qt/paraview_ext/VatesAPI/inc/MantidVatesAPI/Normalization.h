#ifndef MANTID_VATES_NORMALIZATION_H_
#define MANTID_VATES_NORMALIZATION_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

#include <memory>

namespace Mantid {

namespace API {
// Forward declarations
class IMDIterator;
class IMDNode;
class IMDWorkspace;
class IMDEventWorkspace;
}

namespace VATES {

/** Enum describing different ways to normalize the signal
* in a MDWorkspace. We define the VisualNormalization separate from the
MDNormalization because from the
visual perspective we want an AutoSelect option, which is too high level for the
API MDNormalization and will cause
confusion as to it's meaning if left in the core core.
Do not change the enum integers. Adding new options to the enum is ok.
*/
enum VisualNormalization {
  /// Don't normalize = return raw counts
  NoNormalization = 0,
  /// Divide the signal by the volume of the box/bin
  VolumeNormalization = 1,
  /// Divide the signal by the number of events that contributed to it.
  NumEventsNormalization = 2,
  /// Auto select Normalization. We ask the IMDWorkspace to tell us it's
  /// preference
  AutoSelect = 3
};

// Helper typedef
using NormFuncIMDNodePtr = Mantid::signal_t (Mantid::API::IMDNode::*)() const;

/**
Determine which normalization function will be called on an IMDNode
*/
NormFuncIMDNodePtr makeMDEventNormalizationFunction(
    VisualNormalization normalizationOption,
    Mantid::API::IMDEventWorkspace const *const ws);

/**
Determine which normalization function will be called on an IMDIterator of an
IMDWorkspace
*/
DLLExport std::unique_ptr<Mantid::API::IMDIterator>
createIteratorWithNormalization(const VisualNormalization normalizationOption,
                                Mantid::API::IMDWorkspace const *const ws);
}
}

#endif /*MANTID_VATES_NORMALIZATION_H_*/
