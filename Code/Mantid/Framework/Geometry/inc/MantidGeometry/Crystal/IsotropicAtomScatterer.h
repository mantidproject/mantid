#ifndef MANTID_GEOMETRY_ISOTROPICATOMSCATTERER_H_
#define MANTID_GEOMETRY_ISOTROPICATOMSCATTERER_H_

#include "MantidGeometry/Crystal/RigidAtomScatterer.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid
{
namespace Geometry
{

/** IsotropicAtomScatterer : TODO: DESCRIPTION

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class MANTID_GEOMETRY_DLL IsotropicAtomScatterer : public RigidAtomScatterer
{
public:
    IsotropicAtomScatterer(const std::string &element, const Kernel::V3D &position, const UnitCell &cell, double U, double occupancy = 1.0);
    virtual ~IsotropicAtomScatterer() { }

    void setU(double U);
    double getU() const;

    void setCell(const UnitCell &cell);
    UnitCell getCell() const;

    StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const;

protected:
    double m_U;
    UnitCell m_cell;
};


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_ISOTROPICATOMSCATTERER_H_ */
