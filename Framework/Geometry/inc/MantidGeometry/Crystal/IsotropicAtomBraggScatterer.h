#ifndef MANTID_GEOMETRY_ISOTROPICATOMBRAGGSCATTERER_H_
#define MANTID_GEOMETRY_ISOTROPICATOMBRAGGSCATTERER_H_

#include "MantidGeometry/Crystal/BraggScattererInCrystalStructure.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/NeutronAtom.h"

namespace Mantid {
namespace Geometry {

class IsotropicAtomBraggScatterer;

typedef boost::shared_ptr<IsotropicAtomBraggScatterer>
    IsotropicAtomBraggScatterer_sptr;

/** @class IsotropicAtomBraggScatterer

    IsotropicAtomBraggScatterer calculates the structure factor for
    a given HKL using the following equation, which gives the
    structure factor for the j-th atom in the unit cell:

    \f[
        F(hkl)_j = b_j \cdot o_j \cdot DWF_j(hkl) \cdot
            \exp\left[2\pi i \cdot
                \left(h\cdot x_j + k\cdot y_j + l\cdot z_j\right)\right]
    \f]

    Since there are many terms in that equation, further explanation
    is required. The j-th atom in a unit cell occupies a certain position,
    here denoted by the fractional coordinates (x, y, z). With this information,
    an important calculation can be already carried out, calculation of the
    so called "phase". This term is easier seen when the complex part is written
    using trigonometric functions:

    \f{eqnarray*}{
        \phi &=& 2\pi\left(h\cdot x_j + k\cdot y_j + l\cdot z_j\right) \\
        \exp i\phi &=& \cos\phi + i\sin\phi
    \f}

    The magnitude of the complex number is determined first of all by the
    scattering length, \f$b_j\f$ (which is element specific and tabulated, in
    Mantid this is in PhysicalConstants::NeutronAtom). It is multiplied
    by the occupancy\f$o_j\f$, which is a number on the interval [0, 1], where 0
    is a bit meaningless, since it means "no atoms on this position" and
    1 represents a fully occupied position in the crystal lattice. This number
    can be used to model statistically distributed defects.

    \f$DWF_j\f$ denotes the Debye-Waller-factor, which models the diminishing
    scattering power of atoms that are displaced from their position
    (either due to temperature or other effects). It is defined like this:

    \f[
        DWF_j(hkl) = \exp\left[-2\pi^2\cdot U \cdot 1/d_{hkl}^2\right]
    \f]

    Here, \f$U\f$ is given in \f$\mathrm{\AA{}^2}\f$ (for a discussion of
    terms regarding atomic displacement parameters, please see [1]),
    it is often of the order 0.05. \f$d_{hkl}\f$ is alculated using
    the unit cell. The model used in this class is isotropic
    (hence the class name), which may be insufficient depending on the crystal
    structure, but as a first approximation it is often enough.

    This class is designed to handle atoms in a unit cell. When a position is
    set, the internally stored space group is used to generate all positions
    that are symmetrically equivalent. In the structure factor calculation
    method all contributions are summed.

    Easiest is demonstration by example. Copper crystallizes in the space group
    \f$Fm\bar{3}m\f$, Cu atoms occupy the position (0,0,0) and, because
    of the F-centering, also 3 additional positions.

        BraggScatterer_sptr cu =
            BraggScattererFactory::Instance().createScatterer(
                "IsotropicAtomBraggScatterer",
                "Element=Cu; SpaceGroup=F m -3 m")

        cu->setProperty("UnitCell", unitCellToStr(cellCu));
        StructureFactor F = cu->calculateStructureFactor(V3D(1, 1, 1));

    The structure factor F contains contributions from all 4 copper atoms in the
    cell. This is convenient especially for general positions.
    The general position of \f$Fm\bar{3}m\f$ for example has 192 equivalents.

    [1] http://ww1.iucr.org/comm/cnom/adp/finrep/finrep.html

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
class MANTID_GEOMETRY_DLL IsotropicAtomBraggScatterer
    : public BraggScattererInCrystalStructure {
public:
  IsotropicAtomBraggScatterer();
  virtual ~IsotropicAtomBraggScatterer() {}

  std::string name() const { return "IsotropicAtomBraggScatterer"; }
  BraggScatterer_sptr clone() const;

  std::string getElement() const;
  PhysicalConstants::NeutronAtom getNeutronAtom() const;

  double getOccupancy() const;
  double getU() const;

  StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const;

protected:
  void setElement(const std::string &element);

  void declareScattererProperties();
  void afterScattererPropertySet(const std::string &propertyName);

  double getDebyeWallerFactor(const Kernel::V3D &hkl) const;
  double getScatteringLength() const;

  PhysicalConstants::NeutronAtom m_atom;
  std::string m_label;
};

typedef boost::shared_ptr<IsotropicAtomBraggScatterer>
    IsotropicAtomBraggScatterer_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_ISOTROPICATOMBRAGGSCATTERER_H_ */
