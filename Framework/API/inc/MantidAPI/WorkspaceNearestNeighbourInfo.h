// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/V3D.h"

#include <map>
#include <memory>

namespace Mantid {
namespace Geometry {
class IDetector;
}
namespace API {

class MatrixWorkspace;
class WorkspaceNearestNeighbours;

/** WorkspaceNearestNeighbourInfo provides easy access to nearest-neighbour
  information for a workspace.
*/
class MANTID_API_DLL WorkspaceNearestNeighbourInfo {
public:
  WorkspaceNearestNeighbourInfo(const MatrixWorkspace &workspace, const bool ignoreMaskedDetectors,
                                const int nNeighbours = 8);
  ~WorkspaceNearestNeighbourInfo();

  std::map<specnum_t, Kernel::V3D> getNeighbours(const Geometry::IDetector *comp, const double radius = 0.0) const;
  std::map<specnum_t, Kernel::V3D> getNeighbours(specnum_t spec, const double radius) const;
  std::map<specnum_t, Kernel::V3D> getNeighboursExact(specnum_t spec) const;

private:
  const MatrixWorkspace &m_workspace;
  std::unique_ptr<WorkspaceNearestNeighbours> m_nearestNeighbours;
};

} // namespace API
} // namespace Mantid
