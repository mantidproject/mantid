#ifndef MANTID_GEOMETRY_REFLECTIONGENERATOR_H_
#define MANTID_GEOMETRY_REFLECTIONGENERATOR_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"
#include "MantidGeometry/Crystal/HKLFilter.h"

namespace Mantid {
namespace Geometry {

/** ReflectionGenerator : TODO: DESCRIPTION

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
class MANTID_GEOMETRY_DLL ReflectionGenerator {
public:
  enum struct DefaultReflectionConditionFilter {
    None,
    Centering,
    SpaceGroup,
    StructureFactor
  };

  ReflectionGenerator(const CrystalStructure &crystalStructure,
                      DefaultReflectionConditionFilter defaultFilter =
                          DefaultReflectionConditionFilter::SpaceGroup);
  ~ReflectionGenerator() {}

  const CrystalStructure &getCrystalStructure() const;

  std::vector<Kernel::V3D> getHKLs(double dMin, double dMax) const;
  std::vector<Kernel::V3D>
  getHKLs(double dMin, double dMax,
          HKLFilter_const_sptr reflectionConditionFilter) const;

  std::vector<Kernel::V3D> getUniqueHKLs(double dMin, double dMax) const;
  std::vector<Kernel::V3D>
  getUniqueHKLs(double dMin, double dMax,
                HKLFilter_const_sptr reflectionConditionFilter) const;

  std::vector<double> getDValues(const std::vector<Kernel::V3D> &hkls) const;
  std::vector<double> getFsSquared(const std::vector<Kernel::V3D> &hkls) const;

private:
  HKLFilter_const_sptr getDRangeFilter(double dMin, double dMax) const;
  HKLFilter_const_sptr
  getReflectionConditionFilter(DefaultReflectionConditionFilter filter);

  double getD(const Kernel::V3D &hkl) const;

  CrystalStructure m_crystalStructure;
  StructureFactorCalculator_sptr m_sfCalculator;
  HKLFilter_const_sptr m_defaultHKLFilter;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_REFLECTIONGENERATOR_H_ */
