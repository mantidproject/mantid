// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

/** StructureFactorCalculator

 This is a base class for concrete structure factor calculators. It is used
 to logically separate this calculation from CrystalStructure so that different
 methods of calculation can be used. For actual implementations please consult
 the available sub-classes.

      @author Michael Wedel, ESS
      @date 05/09/2015
*/
class MANTID_GEOMETRY_DLL StructureFactorCalculator {
public:
  virtual ~StructureFactorCalculator() = default;

  void setCrystalStructure(const CrystalStructure &crystalStructure);

  /// In implementations this method should return the structure factor for the
  /// specified HKL.
  virtual StructureFactor getF(const Kernel::V3D &hkl) const = 0;
  virtual double getFSquared(const Kernel::V3D &hkl) const;

  virtual std::vector<StructureFactor> getFs(const std::vector<Kernel::V3D> &hkls) const;
  virtual std::vector<double> getFsSquared(const std::vector<Kernel::V3D> &hkls) const;

protected:
  virtual void crystalStructureSetHook(const CrystalStructure &crystalStructure);
};

using StructureFactorCalculator_sptr = std::shared_ptr<StructureFactorCalculator>;

namespace StructureFactorCalculatorFactory {
/// Small templated factory function that creates the desired calculator
/// and initializes it by setting the crystal structure.
template <typename T> StructureFactorCalculator_sptr create(const CrystalStructure &crystalStructure) {
  std::shared_ptr<T> calculator = std::make_shared<T>();
  calculator->setCrystalStructure(crystalStructure);

  return calculator;
}
} // namespace StructureFactorCalculatorFactory

} // namespace Geometry
} // namespace Mantid
