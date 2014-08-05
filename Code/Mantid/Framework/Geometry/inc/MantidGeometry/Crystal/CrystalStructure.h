#ifndef MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_
#define MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Geometry
{

/** CrystalStructure :

    This is a very basic class for holding the parts of a crystal structure.
    Since not all components are present in Mantid (yet), it currently
    comprises a UnitCell, a PointGroup and a ReflectionCondition-object to
    reflect lattice centering.
    
      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 05/08/2014

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
class DLLExport CrystalStructure
{
public:
    CrystalStructure(const UnitCell &unitCell,
                     const PointGroup_sptr &pointGroup = PointGroup_sptr(new PointGroupLaue1),
                     const ReflectionCondition_sptr &centering = ReflectionCondition_sptr(new ReflectionConditionPrimitive));

    virtual ~CrystalStructure() { }

    UnitCell cell() const;
    void setCell(const UnitCell &cell);

    PointGroup_sptr pointGroup() const;
    void setPointGroup(const PointGroup_sptr &pointGroup);
    PointGroup::CrystalSystem crystalSystem() const;

    ReflectionCondition_sptr centering() const;
    void setCentering(const ReflectionCondition_sptr &centering);

    std::vector<Kernel::V3D> getHKLs(double dMin, double dMax) const;
    std::vector<Kernel::V3D> getUniqueHKLs(double dMin, double dMax) const;

    std::vector<double> getDValues(const std::vector<Kernel::V3D> &hkls) const;

protected:
    UnitCell m_cell;
    PointGroup_sptr m_pointGroup;
    ReflectionCondition_sptr m_centering;
    
};

typedef boost::shared_ptr<CrystalStructure> CrystalStructure_sptr;


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_CRYSTALSTRUCTURE_H_ */
