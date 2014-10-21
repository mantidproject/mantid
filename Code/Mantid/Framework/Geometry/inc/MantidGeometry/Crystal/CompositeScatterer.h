#ifndef MANTID_GEOMETRY_SCATTERERCOLLECTION_H_
#define MANTID_GEOMETRY_SCATTERERCOLLECTION_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/IScatterer.h"


namespace Mantid
{
namespace Geometry
{

/** CompositeScatterer

    CompositeScatterer accumulates scatterers, for easier calculation
    of structure factors. Scatterers can be added through the method
    addScatterer. The supplied scatterer is not stored directly,
    it is cloned instead, so there is a new instance. The original instance
    is not modified at all.

    This is important for the behavior of setCell and setSpaceGroup. They
    propagate the supplied value to all internally stored scatterers. This
    makes sense from a crystallographic point of view, since a group of scatterers
    can not contain members that belong to a different crystal structure.

    For structure factor calculations, all contributions from contained scatterers
    are summed. Contained scatterers may be CompositeScatterers themselves,
    so it's possible to build up elaborate structures.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 21/10/2014

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
