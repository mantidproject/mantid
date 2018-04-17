#ifndef MANTID_GEOMETRY_STRUCTUREFACTORCALCULATOR_H_
#define MANTID_GEOMETRY_STRUCTUREFACTORCALCULATOR_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"

namespace Mantid {
namespace Geometry {

/** StructureFactorCalculator

 This is a base class for concrete structure factor calculators. It is used
 to logically separate this calculation from CrystalStructure so that different
 methods of calculation can be used. For actual implementations please consult
 the available sub-classes.

      @author Michael Wedel, ESS
      @date 05/09/2015

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
class MANTID_GEOMETRY_DLL StructureFactorCalculator {
public:
  virtual ~StructureFactorCalculator() = default;

  void setCrystalStructure(const CrystalStructure &crystalStructure);

  /// In implementations this method should return the structure factor for the
  /// specified HKL.
  virtual StructureFactor getF(const Kernel::V3D &hkl) const = 0;
  virtual double getFSquared(const Kernel::V3D &hkl) const;

  virtual std::vector<StructureFactor>
  getFs(const std::vector<Kernel::V3D> &hkls) const;
  virtual std::vector<double>
  getFsSquared(const std::vector<Kernel::V3D> &hkls) const;

protected:
  virtual void
  crystalStructureSetHook(const CrystalStructure &crystalStructure);
};

using StructureFactorCalculator_sptr =
    boost::shared_ptr<StructureFactorCalculator>;

namespace StructureFactorCalculatorFactory {
/// Small templated factory function that creates the desired calculator
/// and initializes it by setting the crystal structure.
template <typename T>
StructureFactorCalculator_sptr
create(const CrystalStructure &crystalStructure) {
  boost::shared_ptr<T> calculator = boost::make_shared<T>();
  calculator->setCrystalStructure(crystalStructure);

  return calculator;
}
}

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_STRUCTUREFACTORCALCULATOR_H_ */
