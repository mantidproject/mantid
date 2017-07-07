#ifndef MANTID_API_NEARESTNEIGHBOURINFO_H_
#define MANTID_API_NEARESTNEIGHBOURINFO_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/V3D.h"

#include <memory>
#include <map>

namespace Mantid {
namespace Geometry {
class IDetector;
}
namespace API {

class MatrixWorkspace;
class WorkspaceNearestNeighbours;

/** WorkspaceNearestNeighbourInfo provides easy access to nearest-neighbour
  information for a workspace.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL WorkspaceNearestNeighbourInfo {
public:
  WorkspaceNearestNeighbourInfo(const MatrixWorkspace &workspace,
                                const bool ignoreMaskedDetectors,
                                const int nNeighbours = 8);
  ~WorkspaceNearestNeighbourInfo();

  std::map<specnum_t, Kernel::V3D>
  getNeighbours(const Geometry::IDetector *comp,
                const double radius = 0.0) const;
  std::map<specnum_t, Kernel::V3D> getNeighbours(specnum_t spec,
                                                 const double radius) const;
  std::map<specnum_t, Kernel::V3D> getNeighboursExact(specnum_t spec) const;

private:
  const MatrixWorkspace &m_workspace;
  std::unique_ptr<WorkspaceNearestNeighbours> m_nearestNeighbours;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACENEARESTNEIGHBOURINFO_H_ */
