#ifndef MANTID_GEOMETRY_BASICHKLFILTERS_H_
#define MANTID_GEOMETRY_BASICHKLFILTERS_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/HKLFilter.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace Geometry {

/** BasicHKLFilters

  This file contains some basic implementations of HKLFilter that
  provide filtering based on things like d-value, space group
  or centering.

      @author Michael Wedel, ESS
      @date 23/09/2015

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL HKLFilterDRange : public HKLFilter {
public:
  HKLFilterDRange(const UnitCell &cell, double dMin, double dMax)
      : m_cell(cell), m_dmin(dMin), m_dmax(dMax) {}

  std::string getName() const { return "dRange"; }

  bool isAllowed(const Kernel::V3D &hkl) const {
    double d = m_cell.d(hkl);

    return d >= m_dmin && d <= m_dmax;
  }

protected:
  UnitCell m_cell;
  double m_dmin, m_dmax;
};

class MANTID_GEOMETRY_DLL HKLFilterSpaceGroup : public HKLFilter {
public:
  HKLFilterSpaceGroup(const SpaceGroup_const_sptr &spaceGroup)
      : m_spaceGroup(spaceGroup) {}

  std::string getName() const { return "SpaceGroup"; }

  bool isAllowed(const Kernel::V3D &hkl) const {
    return m_spaceGroup->isAllowedReflection(hkl);
  }

protected:
  SpaceGroup_const_sptr m_spaceGroup;
};

class MANTID_GEOMETRY_DLL HKLFilterStructureFactor : public HKLFilter {
public:
  HKLFilterStructureFactor(const StructureFactorCalculator_sptr &calculator);

  std::string getName() const { return "SF"; }

  bool isAllowed(const Kernel::V3D &hkl) const;

protected:
  StructureFactorCalculator_sptr m_calculator;
};

class MANTID_GEOMETRY_DLL HKLFilterCentering : public HKLFilter {
public:
  HKLFilterCentering(const ReflectionCondition_sptr &centering)
      : m_centering(centering) {}

  std::string getName() const { return "Centering"; }

  bool isAllowed(const Kernel::V3D &hkl) const {
    return m_centering->isAllowed(static_cast<int>(hkl.X()),
                                  static_cast<int>(hkl.Y()),
                                  static_cast<int>(hkl.Z()));
  }

protected:
  ReflectionCondition_sptr m_centering;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_BASICHKLFILTERS_H_ */
