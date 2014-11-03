#ifndef MANTID_GEOMETRY_COMPOSITEBRAGGSCATTERER_H_
#define MANTID_GEOMETRY_COMPOSITEBRAGGSCATTERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/BraggScatterer.h"


namespace Mantid
{
namespace Geometry
{

/** CompositeBraggScatterer

    CompositeBraggScatterer accumulates scatterers, for easier calculation
    of structure factors. Scatterers can be added through the method
    addScatterer. The supplied scatterer is not stored directly,
    it is cloned instead, so there is a new instance. The original instance
    is not modified at all.

    This is important for the behavior of setCell and setSpaceGroup. They
    propagate the supplied value to all internally stored scatterers. This
    makes sense from a crystallographic point of view, since a group of scatterers
    can not contain members that belong to a different crystal structure.

    For structure factor calculations, all contributions from contained scatterers
    are summed. Contained scatterers may be CompositeBraggScatterers themselves,
    so it's possible to build up elaborate structures.

    There are two ways of creating instances of CompositeBraggScatterer. The first
    possibility is to use BraggScattererFactory, just like for other implementations
    of BraggScatterer. Additionally there is a static method CompositeBraggScatterer::create,
    which creates a composite scatterer of the supplied vector of scatterers.

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
class CompositeBraggScatterer;

typedef boost::shared_ptr<CompositeBraggScatterer> CompositeBraggScatterer_sptr;

class MANTID_GEOMETRY_DLL CompositeBraggScatterer : public BraggScatterer
{
public:
    CompositeBraggScatterer();
    virtual ~CompositeBraggScatterer() { }

    static CompositeBraggScatterer_sptr create();
    static CompositeBraggScatterer_sptr create(const std::vector<BraggScatterer_sptr> &scatterers);

    std::string name() const { return "CompositeBraggScatterer"; }
    BraggScatterer_sptr clone() const;

    void addScatterer(const BraggScatterer_sptr &scatterer);
    size_t nScatterers() const;
    BraggScatterer_sptr getScatterer(size_t i) const;
    void removeScatterer(size_t i);

    StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const;
    
protected:
    void afterScattererPropertySet(const std::string &propertyName);
    void propagateProperty(const std::string &propertyName);

    void setCommonProperties(BraggScatterer_sptr &scatterer);

    std::vector<BraggScatterer_sptr> m_scatterers;
};



} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_COMPOSITEBRAGGSCATTERER_H_ */
