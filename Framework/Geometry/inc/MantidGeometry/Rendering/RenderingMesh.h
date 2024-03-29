// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Geometry {

/** RenderingMesh : Mesh abstraction required for rendering
 */
class MANTID_GEOMETRY_DLL RenderingMesh {

public:
  virtual size_t numberOfVertices() const = 0;
  virtual size_t numberOfTriangles() const = 0;
  virtual std::vector<double> getVertices() const = 0;
  virtual std::vector<uint32_t> getTriangles() const = 0;
  virtual ~RenderingMesh() = default;
};

} // namespace Geometry
} // namespace Mantid
