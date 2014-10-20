#ifndef MANTID_GEOMETRY_RIGIDATOMSCATTERER_H_
#define MANTID_GEOMETRY_RIGIDATOMSCATTERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/IScatterer.h"
#include "MantidKernel/NeutronAtom.h"

namespace Mantid
{
namespace Geometry
{

/** RigidAtomScatterer : TODO: DESCRIPTION

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
class MANTID_GEOMETRY_DLL RigidAtomScatterer : public IScatterer
{
public:
    RigidAtomScatterer(const std::string &element, const Kernel::V3D &position, double occupancy = 1.0);
    virtual ~RigidAtomScatterer() { }

    void setElement(const std::string &element);
    std::string getElement() const;
    PhysicalConstants::NeutronAtom getNeutronAtom() const;

    void setOccupancy(double occupancy);
    double getOccupancy() const;

    StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const;

    
protected:
    PhysicalConstants::NeutronAtom m_atom;
    std::string m_label;
    double m_occupancy;
};

} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_RIGIDATOMSCATTERER_H_ */
