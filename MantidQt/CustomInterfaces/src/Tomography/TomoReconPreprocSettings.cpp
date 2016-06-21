#include "MantidQtCustomInterfaces/Tomography/TomoReconPreprocSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Default initialization of pre-processing options. This should
 * represent sensible first time usage defaults that can be used by
 * GUIs, etc.
 */
TomoReconPreprocSettings::TomoReconPreprocSettings()
    : normalizeByAirRegion(true), normalizeByProtonCharge(false),
      normalizeByFlats(true), normalizeByDarks(true), medianFilterWidth(3),
      rotation(0), maxAngle(360.0), scaleDownFactor(0) {}

} // namespace CustomInterfaces
} // namespace MantidQt
