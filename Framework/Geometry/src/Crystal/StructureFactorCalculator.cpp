// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"

namespace Mantid {
namespace Geometry {

/**
 * Sets the crystal structure for which to calculate structure factors
 *
 * Additionally StructureFactorCalculator::crystalStructureSetHook() is called
 * with the CrystalStructure. This function may be re-implemented
 * by concrete structure factor calculators to perform other necessary actions
 * that involve the crystal structure (for example extracting and storing
 * members of it).
 *
 * @param crystalStructure :: Crystal structure for calculations.
 */
void StructureFactorCalculator::setCrystalStructure(
    const CrystalStructure &crystalStructure) {
  crystalStructureSetHook(crystalStructure);
}

/// Returns F^2 for the given HKL, calling StructureFactorCalculator::getF().
double StructureFactorCalculator::getFSquared(const Kernel::V3D &hkl) const {
  StructureFactor sf = getF(hkl);

  return sf.real() * sf.real() + sf.imag() * sf.imag();
}

/**
 * Returns structure factors for each HKL in the container
 *
 * The default implementation uses StructureFactorCalculator::getF() to get
 * the structure factor for each HKL. This behavior can be overriden in
 * sub-classes for example to implement this in a more efficient way.
 *
 * @param hkls :: Vector of HKLs.
 * @return :: Vector of structure factors for the given HKLs.
 */
std::vector<StructureFactor>
StructureFactorCalculator::getFs(const std::vector<Kernel::V3D> &hkls) const {
  std::vector<StructureFactor> structureFactors(hkls.size());
  using namespace std::placeholders;
  std::transform(hkls.begin(), hkls.end(), structureFactors.begin(),
                 std::bind(&StructureFactorCalculator::getF, this, _1));

  return structureFactors;
}

/**
 * Returns structure factors for each HKL in the container
 *
 * The default implementation uses StructureFactorCalculator::getFSquared() with
 * each HKL. This behavior can be overriden in
 * sub-classes for example to implement this in a more efficient way.
 *
 * @param hkls :: Vector of HKLs.
 * @return :: Vector of squared structure factors for the given HKLs.
 */
std::vector<double> StructureFactorCalculator::getFsSquared(
    const std::vector<Kernel::V3D> &hkls) const {
  std::vector<double> fSquareds(hkls.size());
  using namespace std::placeholders;
  std::transform(hkls.begin(), hkls.end(), fSquareds.begin(),
                 std::bind(&StructureFactorCalculator::getFSquared, this, _1));

  return fSquareds;
}

/// This function is called from
/// StructureFactorCalculator::setCrystalStructure() and can be overriden to
/// perform additional actions.
void StructureFactorCalculator::crystalStructureSetHook(
    const CrystalStructure &crystalStructure) {
  UNUSED_ARG(crystalStructure)
}

} // namespace Geometry
} // namespace Mantid
