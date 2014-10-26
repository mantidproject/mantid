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

    IScatterer is a general interface for representing scatterers
    in the unit cell of a periodic structure. Since there are many possibilities
    of modelling scatterers, IScatterer is derived from PropertyManager.
    This way, new scatterers with very different parameters can be
    added easily.

    In IScatterer, three basic properties are defined: Position, UnitCell
    and SpaceGroup. Setting these properties is only possible through
    the setProperty and setPropertyValue methods inherited from
    PropertyManager. For retrieval there are however specialized methods,
    since UnitCell and SpaceGroup properties are currently only stored
    as strings.

    New implementations must override the declareProperties method and
    define any parameters there. It is called by the initialize method.
    Currently there are two implementations of this interface, IsotropicAtomScatterer
    and CompositeScatterer (composite pattern) - especially the latter
    provides a good example on how to define new properties for a scatterer.

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

class IScatterer;

typedef boost::shared_ptr<IScatterer> IScatterer_sptr;

class MANTID_GEOMETRY_DLL IScatterer : public Kernel::PropertyManager
{
public:
    IScatterer();
    virtual ~IScatterer() { }

    void initialize();
    bool isInitialized();

    virtual std::string name() const = 0;
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

    void afterPropertySet(const std::string &propertyName);

    /// Base implementation does nothing - for implementing classes only.
    virtual void declareProperties() { }

    /// This method should be re-implemented by subclasses for additional parameter processing.
    virtual void afterScattererPropertySet(const std::string &) { }

    void recalculateEquivalentPositions();

    Kernel::V3D m_position;
    std::vector<Kernel::V3D> m_equivalentPositions;

    UnitCell m_cell;
    SpaceGroup_const_sptr m_spaceGroup;

    bool m_isInitialized;
};

/**
 * Helper class for validating unit cell strings.
 *
 * This validator checks whether a string consists of either 3 or 6 numbers,
 * possibly floating point numbers. It's required for the unit cell string
 * property.
 */
class MANTID_GEOMETRY_DLL UnitCellStringValidator : public Kernel::TypedValidator<std::string>
{
protected:
    Kernel::IValidator_sptr clone() const;
    virtual std::string checkValidity(const std::string &unitCellString) const;
};

} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_ISCATTERER_H_ */
