#ifndef MANTID_GEOMETRY_ISCATTERER_H_
#define MANTID_GEOMETRY_ISCATTERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"

#include <complex>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Geometry
{

typedef std::complex<double> StructureFactor;

/** IScatterer

    General interface for any kind of scatterer. Position is expected to
    be set as fractional coordinates with respect to the crystal axes.

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

class MANTID_GEOMETRY_DLL IScatterer
{
public:
    IScatterer(const Kernel::V3D &position = Kernel::V3D(0.0, 0.0, 0.0));
    virtual ~IScatterer() { }

    virtual IScatterer_sptr clone() const = 0;

    virtual void setPosition(const Kernel::V3D &position);
    Kernel::V3D getPosition() const;
    std::vector<Kernel::V3D> getEquivalentPositions() const;

    virtual void setCell(const UnitCell &cell);
    UnitCell getCell() const;

    virtual void setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);
    SpaceGroup_const_sptr getSpaceGroup() const;

    virtual StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const = 0;
    
protected:
    void recalculateEquivalentPositions();

    Kernel::V3D m_position;
    std::vector<Kernel::V3D> m_equivalentPositions;

    UnitCell m_cell;
    SpaceGroup_const_sptr m_spaceGroup;
};

} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_ISCATTERER_H_ */
