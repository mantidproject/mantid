#ifndef MANTID_SINQ_POLDICREATEPEAKSFROMCELL_H_
#define MANTID_SINQ_POLDICREATEPEAKSFROMCELL_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
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
  PoldiCreatePeaksFromCell();
  virtual ~PoldiCreatePeaksFromCell();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

  virtual std::map<std::string, std::string> validateInputs();

protected:
  Geometry::SpaceGroup_const_sptr
  getSpaceGroup(const std::string &spaceGroupString) const;

  Geometry::CompositeBraggScatterer_sptr
  getScatterers(const std::string &scattererString) const;
  Geometry::BraggScatterer_sptr
  getScatterer(const std::string &singleScatterer) const;
  std::vector<std::string>
  getCleanScattererTokens(const std::vector<std::string> &tokens) const;

  double getDMaxValue(const Geometry::UnitCell &unitCell) const;

  double getLargestDValue(const Geometry::UnitCell &unitCell) const;

  Geometry::UnitCell getUnitCellFromProperties() const;
  Geometry::UnitCell getConstrainedUnitCell(
      const Geometry::UnitCell &unitCell,
      const Geometry::PointGroup::CrystalSystem &crystalSystem) const;

private:
  void init();
  void exec();
};

} // namespace SINQ
} // namespace Mantid

#endif /* MANTID_SINQ_POLDICREATEPEAKSFROMCELL_H_ */
