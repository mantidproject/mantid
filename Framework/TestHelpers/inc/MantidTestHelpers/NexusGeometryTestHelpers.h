// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef NEXUSGEOMETRYTESTHELPERS_H
#define NEXUSGEOMETRYTESTHELPERS_H

#include <boost/shared_ptr.hpp>
#include <vector>

#include "MantidNexusGeometry/TubeHelpers.h"

namespace Mantid {
namespace Geometry {
class IObject;
}
} // namespace Mantid

namespace NexusGeometryTestHelpers {
boost::shared_ptr<const Mantid::Geometry::IObject> createShape();
Pixels generateCoLinearPixels();
Pixels generateNonCoLinearPixels();
std::vector<int> getFakeDetIDs();
} // namespace NexusGeometryTestHelpers

#endif // NEXUSGEOMETRYTESTHELPERS_H