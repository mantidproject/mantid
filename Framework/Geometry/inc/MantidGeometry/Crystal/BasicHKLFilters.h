#ifndef MANTID_GEOMETRY_BASICHKLFILTERS_H_
#define MANTID_GEOMETRY_BASICHKLFILTERS_H_

#include "MantidGeometry/Crystal/HKLFilter.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/DllConfig.h"

#include <strstream>

namespace Mantid {
namespace Geometry {

/** BasicHKLFilters

  This file contains some implementations of HKLFilter that
  provide filtering based on things like d-value, space group
  or centering.

  A common use would be to generate a specific list of HKLs,
  for example all reflections that are allowed according to a certain
  range of d-values and the reflection conditions of a space group:

    HKLFilter_const_sptr filter =
        boost::make_shared<HKLFilterDRange>(unitCell, 0.5)
      & boost::make_shared<HKLFilterSpaceGroup>(spaceGroup);

    HKLGenerator gen(unitCell, 0.5);
    std::vector<V3D> hkls;

    std::remove_copy_if(gen.begin(), gen.end(), std::back_inserter(hkls),
                        (~filter)->fn());

  An existing list of HKLs could be checked for indices that match the
  reflection conditions of a space group:

    HKLFilter_const_sptr sgFilter =
        boost::make_shared<HKLFilterSpaceGroup>(spaceGroup);

    auto matchingHKLCount = std::count_if(hkls.begin(), hkls.end(),
                                          sgFilter->fn());

    auto violatingHKLCount = std::count_if(hkls.begin(), hkls.end(),
                                          (~sgFilter)->fn());

  Combining HKLGenerator and different HKLFilters provides a very flexible
  system for creating and processing specific sets of Miller indices that
  is easy to expand by adding other HKLFilters.

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

/**
 * A class to filter HKLs by their d-values
 *
 * This class takes a UnitCell object and calculates the spacing of
 * the lattice planes for each HKL. If the lattice spacing is within
 * the spcified range of values, the reflection is allowed.
 *
 * If the first constructor with only dMin is used, dMax is taken to
 * be the lattice parameter with the largest value. There can not be
 * a greater interplanar spacing than that value.
 */
class MANTID_GEOMETRY_DLL HKLFilterDRange : public HKLFilter {
public:
  HKLFilterDRange(const UnitCell &cell, double dMin);
  HKLFilterDRange(const UnitCell &cell, double dMin, double dMax);

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;

private:
  void checkProperDRangeValues();

  UnitCell m_cell;
  double m_dmin, m_dmax;
};

/**
 * A class to filter HKLs according to a space group
 *
 * HKLFilterSpaceGroup stores a SpaceGroup object and marks those
 * reflections as allowed that are allowed according to the
 * reflection conditions of the space group.
 */
class MANTID_GEOMETRY_DLL HKLFilterSpaceGroup : public HKLFilter {
public:
  HKLFilterSpaceGroup(const SpaceGroup_const_sptr &spaceGroup);

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;

protected:
  SpaceGroup_const_sptr m_spaceGroup;
};

/**
 * A class to filter HKLs according to structure factor magnitudes
 *
 * This filter takes a StructureFactorCalculator and calculates the
 * structure factor for each HKL. If F^2 is larger than the specified
 * minimum, the reflection is considered allowed. The default minimum
 * is 1e-6.
 */
class MANTID_GEOMETRY_DLL HKLFilterStructureFactor : public HKLFilter {
public:
  HKLFilterStructureFactor(const StructureFactorCalculator_sptr &calculator,
                           double fSquaredMin = 1.0e-6);

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;

protected:
  StructureFactorCalculator_sptr m_calculator;
  double m_fSquaredMin;
};

/**
 * A class to filter HKLs according to a lattice centering
 *
 * HKLFilterCentering is a filter that stores a ReflectionCondition object
 * internally and filters the HKLs according to that.
 */
class MANTID_GEOMETRY_DLL HKLFilterCentering : public HKLFilter {
public:
  HKLFilterCentering(const ReflectionCondition_sptr &centering);

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;

protected:
  ReflectionCondition_sptr m_centering;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_BASICHKLFILTERS_H_ */
