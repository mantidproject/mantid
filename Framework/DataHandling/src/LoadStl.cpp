// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadStl.h"
#include "MantidKernel/V3D.h"
#include <Poco/File.h>

#include <functional>

namespace Mantid::DataHandling {

bool LoadStl::areEqualVertices(Kernel::V3D const &v1, Kernel::V3D const &v2) const {
  const Kernel::V3D diff = v1 - v2;
  const double nanoMetre = 1e-9;
  return diff.norm() < nanoMetre; // This is 1 nanometre for a unit of a metre.
}

void LoadStl::changeToVector() {
  m_vertices.resize(vertexSet.size());
  for (auto const &mapValue : vertexSet) {
    m_vertices[mapValue.second] = mapValue.first;
  }
}

} // namespace Mantid::DataHandling
