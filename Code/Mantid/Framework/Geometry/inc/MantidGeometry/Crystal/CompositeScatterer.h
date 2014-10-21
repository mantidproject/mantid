#ifndef MANTID_GEOMETRY_SCATTERERCOLLECTION_H_
#define MANTID_GEOMETRY_SCATTERERCOLLECTION_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/IScatterer.h"


namespace Mantid
{
namespace Geometry
{

/** CompositeScatterer : TODO: DESCRIPTION

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
class CompositeScatterer;

typedef boost::shared_ptr<CompositeScatterer> CompositeScatterer_sptr;

class MANTID_GEOMETRY_DLL CompositeScatterer : public IScatterer
{
public:
    CompositeScatterer();
    virtual ~CompositeScatterer() { }

    static CompositeScatterer_sptr create();
    static CompositeScatterer_sptr create(const std::vector<IScatterer_sptr> &scatterers);

    IScatterer_sptr clone() const;

    void setCell(const UnitCell &cell);
    void setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

    void addScatterer(const IScatterer_sptr &scatterer);
    size_t nScatterers() const;
    IScatterer_sptr getScatterer(size_t i) const;
    void removeScatterer(size_t i);

    StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const;
    
protected:
    void setCommonProperties(IScatterer_sptr &scatterer);

    std::vector<IScatterer_sptr> m_scatterers;
};



} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_SCATTERERCOLLECTION_H_ */
