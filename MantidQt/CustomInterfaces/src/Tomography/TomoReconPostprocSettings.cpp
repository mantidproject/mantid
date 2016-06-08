#include "MantidQtCustomInterfaces/Tomography/TomoReconPostprocSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Default initialization of post-processing options. This should
 * represent sensible first time usage defaults that can be used by
 * GUIs, etc.
 */
TomoReconPostprocSettings::TomoReconPostprocSettings()
    : circMaskRadius(0.94), cutOffLevel(0.0) {}

} // namespace CustomInterfaces
} // namespace MantidQt
