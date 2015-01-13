#ifndef MANTID_GEOMETRY_COMPOSITEBRAGGSCATTERER_H_
#define MANTID_GEOMETRY_COMPOSITEBRAGGSCATTERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/BraggScatterer.h"

namespace Mantid {
namespace Geometry {

/** CompositeBraggScatterer

    CompositeBraggScatterer accumulates scatterers, for easier calculation
    of structure factors. Scatterers can be added through the method
    addScatterer. The supplied scatterer is not stored directly,
    it is cloned instead, so there is a new instance. The original instance
    is not modified at all.

    For structure factor calculations, all contributions from contained
   scatterers
    are summed. Contained scatterers may be CompositeBraggScatterers themselves,
    so it's possible to build up elaborate structures.

    There are two ways of creating instances of CompositeBraggScatterer. The
   first
    possibility is to use BraggScattererFactory, just like for other
   implementations
    of BraggScatterer. Additionally there is a static method
   CompositeBraggScatterer::create,
    which creates a composite scatterer of the supplied vector of scatterers.

    CompositeBraggScatterer does not declare any methods by itself, instead it
   exposes
    some properties of the contained scatterers (those which were marked using
    exposePropertyToComposite). When these properties are set, their values
    are propagated to all members of the composite. The default behavior when
    new properties are declared in subclasses of BraggScatterer is not to expose
    them in this way.

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

class MANTID_GEOMETRY_DLL CompositeBraggScatterer : public BraggScatterer {
public:
  CompositeBraggScatterer();
  virtual ~CompositeBraggScatterer() {}

  static CompositeBraggScatterer_sptr create();
  static CompositeBraggScatterer_sptr
  create(const std::vector<BraggScatterer_sptr> &scatterers);

  std::string name() const { return "CompositeBraggScatterer"; }
  BraggScatterer_sptr clone() const;

  void addScatterer(const BraggScatterer_sptr &scatterer);
  size_t nScatterers() const;
  BraggScatterer_sptr getScatterer(size_t i) const;
  void removeScatterer(size_t i);
  void removeAllScatterers();

  StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const;

protected:
  void afterPropertySet(const std::string &propertyName);
  void propagateProperty(const std::string &propertyName);
  void propagatePropertyToScatterer(BraggScatterer_sptr &scatterer,
                                    const std::string &propertyName,
                                    const std::string &propertyValue);

  void redeclareProperties();
  std::map<std::string, size_t> getPropertyCountMap() const;

  std::vector<BraggScatterer_sptr> m_scatterers;
};
}
}

#endif
