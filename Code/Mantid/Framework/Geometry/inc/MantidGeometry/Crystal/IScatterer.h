#ifndef MANTID_GEOMETRY_ISCATTERER_H_
#define MANTID_GEOMETRY_ISCATTERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"

#include <complex>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid
{
namespace Geometry
{

typedef std::complex<double> StructureFactor;

/** IScatterer

    General interface for any kind of scatterer. Position is expected to
    be set as fractional coordinates with respect to the crystal axes.

    Currently there are two implementations of this interface, IsotropicAtomScatterer
    and CompositeScatterer (composite pattern).

    Most of the interface serves the purpose of providing necessary
    infrastructure for calculating structure factors. This includes
    information about unit cell and space group, as well as position
    of the scatterer in the cell.

    Please note the default behavior of the methods setPosition and setSpaceGroup.
    When either of them is called, the equivalent positions are recalculated. For
    more information on how this is done, please consult the documentation
    of SpaceGroup.

    If no space group is set, or it's P1, only one "equivalent" position is
    generated - the position itself.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 20/10/2014

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

class IScatterer;

typedef boost::shared_ptr<IScatterer> IScatterer_sptr;

class MANTID_GEOMETRY_DLL IScatterer : public Kernel::PropertyManager
{
public:
    IScatterer(const Kernel::V3D &position = Kernel::V3D(0.0, 0.0, 0.0));
    virtual ~IScatterer() { }

    void initialize();

    virtual IScatterer_sptr clone() const = 0;

    Kernel::V3D getPosition() const;
    std::vector<Kernel::V3D> getEquivalentPositions() const;
    UnitCell getCell() const;
    SpaceGroup_const_sptr getSpaceGroup() const;

    virtual StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const = 0;
    
protected:
    virtual void setPosition(const Kernel::V3D &position);
    virtual void setCell(const UnitCell &cell);
    virtual void setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

    virtual void declareProperties() { }

    void recalculateEquivalentPositions();

    Kernel::V3D m_position;
    std::vector<Kernel::V3D> m_equivalentPositions;

    UnitCell m_cell;
    SpaceGroup_const_sptr m_spaceGroup;
};

class MANTID_GEOMETRY_DLL UnitCellStringValidator : public Kernel::TypedValidator<std::string>
{
protected:
    Kernel::IValidator_sptr clone() const;
    virtual std::string checkValidity(const std::string &unitCellString) const;
};

} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_ISCATTERER_H_ */
