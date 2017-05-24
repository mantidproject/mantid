#ifndef MANTID_CRYSTAL_COUNTREFLECTIONS_H_
#define MANTID_CRYSTAL_COUNTREFLECTIONS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/PeakStatisticsTools.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {

/** CountReflections

  This algorithm takes a PeaksWorkspace and calculates statistics that are
  based on point group symmetry and do not depend on intensities. For those
  statistics look at SortHKL.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CountReflections : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  API::IPeaksWorkspace_sptr getPeaksWorkspace(
      const DataObjects::PeaksWorkspace_sptr &templateWorkspace,
      const PeakStatisticsTools::UniqueReflectionCollection &reflections,
      const Geometry::PointGroup_sptr &pointGroup) const;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_COUNTREFLECTIONS_H_ */
