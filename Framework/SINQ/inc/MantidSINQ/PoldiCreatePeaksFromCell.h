#ifndef MANTID_SINQ_POLDICREATEPEAKSFROMCELL_H_
#define MANTID_SINQ_POLDICREATEPEAKSFROMCELL_H_

#include "MantidAPI/Algorithm.h"
#include "MantidSINQ/DllConfig.h"

#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace Poldi {

/** PoldiCreatePeaksFromCell :

    This algorithm creates a list of reflections with HKL and d-values
    in the form of a TableWorkspace that can be converted to a
    PoldiPeakCollection.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 16/09/2014

      Copyright © 2014 PSI-MSS

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
class MANTID_SINQ_DLL PoldiCreatePeaksFromCell : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PoldiCreatePeaksFromFile"};
  }
  const std::string category() const override;
  const std::string summary() const override;

  std::map<std::string, std::string> validateInputs() override;

protected:
  Geometry::SpaceGroup_const_sptr
  getSpaceGroup(const std::string &spaceGroupString) const;

  double getDMaxValue(const Geometry::UnitCell &unitCell) const;

  double getLargestDValue(const Geometry::UnitCell &unitCell) const;

  Geometry::UnitCell getUnitCellFromProperties() const;
  Geometry::UnitCell getConstrainedUnitCell(
      const Geometry::UnitCell &unitCell,
      const Geometry::PointGroup::CrystalSystem &crystalSystem,
      const Geometry::Group::CoordinateSystem &coordinateSystem =
          Geometry::Group::Orthogonal) const;

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDICREATEPEAKSFROMCELL_H_ */
