// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"

namespace Mantid {
namespace Geometry {
class ComponentInfo;
class IComponent;
} // namespace Geometry
namespace API {
/** Helpers for resizing RectangularDetectors
 */
MANTID_API_DLL void applyRectangularDetectorScaleToComponentInfo(Geometry::ComponentInfo &componentInfo,
                                                                 Geometry::IComponent *componentId, const double scaleX,
                                                                 const double scaleY);
} // namespace API
} // namespace Mantid
