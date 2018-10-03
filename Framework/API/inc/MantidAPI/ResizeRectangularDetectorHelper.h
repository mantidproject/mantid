// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_RESIZERECTANGULARDETECTORHELPER_H_
#define MANTID_API_RESIZERECTANGULARDETECTORHELPER_H_

#include "MantidAPI/DllConfig.h"

namespace Mantid {
namespace Geometry {
class ComponentInfo;
class IComponent;
} // namespace Geometry
namespace API {
/** Helpers for resizing RectangularDetectors
 */
MANTID_API_DLL void applyRectangularDetectorScaleToComponentInfo(
    Geometry::ComponentInfo &componentInfo, Geometry::IComponent *componentId,
    const double scaleX, const double scaleY);
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_RESIZERECTANGULARDETECTORHELPER_H_ */
