#ifndef MANTID_GEOMETRY_BRAGGSCATTERER_H_
#define MANTID_GEOMETRY_BRAGGSCATTERER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

#include <complex>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace Geometry {

typedef std::complex<double> StructureFactor;

/** BraggScatterer

    BraggScatterer is a general interface for representing scatterers
    in the unit cell of a periodic structure. Since there are many possibilities
    of modelling scatterers, BraggScatterer is derived from PropertyManager.
    This way, new scatterers with very different parameters can be
    added easily.

    New implementations must override the declareProperties method and
    define any parameters there. For most applications it should be easier
    to inherit from BraggScattererInCrystalStructure, which provides some
    default properties that are useful in many cases. CompositeBraggScatterer
    is designed to combine several scatterers.

    CompositeBraggScatterer does not declare any properties by itself. For
    some properties it makes sense to be equal for all scatterers in the
    composite. This behavior can be achieved by calling the method
    makePropertyPropagating after it has been declared. Examples are
    the UnitCell and SpaceGroup properties in BraggScattererInCrystalStructure.

    Construction of concrete scatterers is done through ScattererFactory.

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

class BraggScatterer;

typedef boost::shared_ptr<BraggScatterer> BraggScatterer_sptr;

class MANTID_GEOMETRY_DLL BraggScatterer : public Kernel::PropertyManager {
public:
  BraggScatterer();
  virtual ~BraggScatterer() {}

  void initialize();
  bool isInitialized();

  virtual std::string name() const = 0;
  virtual BraggScatterer_sptr clone() const = 0;

  virtual StructureFactor
  calculateStructureFactor(const Kernel::V3D &hkl) const = 0;
  double calculateFSquared(const Kernel::V3D &hkl) const;

  bool isPropertyExposedToComposite(const std::string &propertyName) const;
  bool isPropertyExposedToComposite(Kernel::Property *property) const;

protected:
  /// Base implementation does nothing, can be re-implemented by subclasses.
  void afterPropertySet(const std::string &) {}

  /// Base implementation does nothing - for implementing classes only.
  virtual void declareProperties() {}

  void exposePropertyToComposite(const std::string &propertyName);
  void unexposePropertyFromComposite(const std::string &propertyName);

  const std::string &getPropagatingGroupName() const;

private:
  std::string m_propagatingGroupName;
  bool m_isInitialized;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_BRAGGSCATTERER_H_ */
