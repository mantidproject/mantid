// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rendering/RenderingHelpers.h"
#include <stdexcept>

namespace {
void throwNoOpenGLError(const std::string &function) {
  throw std::runtime_error(
      function + ": Rendering not supported in a build without OpenGL. Rebuild "
                 "with ENABLE_OPENGL to enable support.");
}

} // namespace

namespace Mantid {
namespace Geometry {
namespace RenderingHelpers {
void renderIObjComponent(const IObjComponent &) {
  throwNoOpenGLError("renderIObjComponent");
}

void renderTriangulated(detail::GeometryTriangulator &) {
  throwNoOpenGLError("renderTriangulated");
}

void renderShape(const detail::ShapeInfo &) {
  throwNoOpenGLError("renderShape");
}

} // namespace RenderingHelpers
} // namespace Geometry
} // namespace Mantid
