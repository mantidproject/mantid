#ifndef MANTID_GEOMETRY_BRAGGSCATTERERINCRYSTALSTRUCTURE_H_
#define MANTID_GEOMETRY_BRAGGSCATTERERINCRYSTALSTRUCTURE_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/BraggScatterer.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"

namespace Mantid {
namespace Geometry {

/**
    @class BraggScattererInCrystalStructure

    This class provides an extension of BraggScatterer, suitable
    for scatterers that are part of a crystal structure. Information about
    the unit cell and space group can be set. The space group information
    is used to calculate equivalent positions in the structure.

    Both space group and unit cell are exposed marked as exposed to
    BraggScattererComposite, so all members of one composite will
    have the same unit cell and space group.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 04/11/2014

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

class MANTID_GEOMETRY_DLL BraggScattererInCrystalStructure
    : public BraggScatterer {
public:
  BraggScattererInCrystalStructure();
  virtual ~BraggScattererInCrystalStructure() {}

  Kernel::V3D getPosition() const;
  std::vector<Kernel::V3D> getEquivalentPositions() const;
  UnitCell getCell() const;
  SpaceGroup_const_sptr getSpaceGroup() const;

protected:
  virtual void afterPropertySet(const std::string &propertyName);

  /// This method should be re-implemented by subclasses for additional
  /// parameter processing.
  virtual void afterScattererPropertySet(const std::string &) {}

  /// This method should be implemented by subclasses for declaring additional
  /// properties.
  virtual void declareScattererProperties() {}

  virtual void setPosition(const Kernel::V3D &position);
  virtual void setCell(const UnitCell &cell);
  virtual void setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

  virtual void declareProperties();

  Kernel::V3D getPositionFromString(const std::string &positionString) const;
  void recalculateEquivalentPositions();

  Kernel::V3D m_position;
  std::vector<Kernel::V3D> m_equivalentPositions;

  UnitCell m_cell;
  SpaceGroup_const_sptr m_spaceGroup;
};

typedef boost::shared_ptr<BraggScattererInCrystalStructure>
    BraggScattererInCrystalStructure_sptr;

/**
 * Helper class for validating unit cell strings.
 *
 * This validator checks whether a string consists of either 3 or 6 numbers,
 * possibly floating point numbers. It's required for the unit cell string
 * property.
 */
class MANTID_GEOMETRY_DLL UnitCellStringValidator
    : public Kernel::TypedValidator<std::string> {
protected:
  Kernel::IValidator_sptr clone() const;
  virtual std::string checkValidity(const std::string &unitCellString) const;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_BRAGGSCATTERERINCRYSTALSTRUCTURE_H_ */
