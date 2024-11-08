// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>
#include <vector>

#include "MantidNexusGeometry/TubeHelpers.h"

namespace Mantid {
namespace Geometry {

class IObject;

}
} // namespace Mantid

namespace NexusGeometryTestHelpers {

std::shared_ptr<const Mantid::Geometry::IObject> createShape();

Pixels generateCoLinearPixels();

Pixels generateNonCoLinearPixels();

std::vector<int> getFakeDetIDs();

} // namespace NexusGeometryTestHelpers
