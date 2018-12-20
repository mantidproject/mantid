#include "MantidGeometry/Crystal/ReflectionGenerator.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/StructureFactorCalculatorSummation.h"

namespace Mantid {
namespace Geometry {

using namespace Kernel;

/// Small helper functor to calculate d-Values from a unit cell.
class LatticeSpacingCalculator {
public:
  explicit LatticeSpacingCalculator(const UnitCell &cell) : m_cell(cell) {}

  double operator()(const V3D &hkl) { return m_cell.d(hkl); }

private:
  UnitCell m_cell;
};

/// Constructor
ReflectionGenerator::ReflectionGenerator(
    const CrystalStructure &crystalStructure,
    ReflectionConditionFilter defaultFilter)
    : m_crystalStructure(crystalStructure),
      m_sfCalculator(StructureFactorCalculatorFactory::create<
                     StructureFactorCalculatorSummation>(m_crystalStructure)),
      m_defaultHKLFilter(getReflectionConditionFilter(defaultFilter)) {}

/// Returns the internally stored crystal structure
const CrystalStructure &ReflectionGenerator::getCrystalStructure() const {
  return m_crystalStructure;
}

/// Returns a DRangeFilter from the supplied d-limits and the internally stored
/// cell.
HKLFilter_const_sptr ReflectionGenerator::getDRangeFilter(double dMin,
                                                          double dMax) const {
  return boost::make_shared<const HKLFilterDRange>(m_crystalStructure.cell(),
                                                   dMin, dMax);
}

/// Returns a reflection condition HKLFilter based on the supplied enum.
HKLFilter_const_sptr ReflectionGenerator::getReflectionConditionFilter(
    ReflectionConditionFilter filter) {
  switch (filter) {
  case ReflectionConditionFilter::Centering:
    return boost::make_shared<const HKLFilterCentering>(
        m_crystalStructure.centering());
    break;
  case ReflectionConditionFilter::SpaceGroup:
    return boost::make_shared<const HKLFilterSpaceGroup>(
        m_crystalStructure.spaceGroup());
    break;
  case ReflectionConditionFilter::StructureFactor:
    return boost::make_shared<const HKLFilterStructureFactor>(m_sfCalculator);
  default:
    return HKLFilter_const_sptr();
  }
}

/// Returns a list of HKLs within the specified d-limits using the default
/// reflection condition filter.
std::vector<V3D> ReflectionGenerator::getHKLs(double dMin, double dMax) const {
  return getHKLs(dMin, dMax, m_defaultHKLFilter);
}

/// Returns a list of HKLs within the specified d-limits using the specified
/// filter. If the pointer is null, it's ignored.
std::vector<Kernel::V3D> ReflectionGenerator::getHKLs(
    double dMin, double dMax,
    HKLFilter_const_sptr reflectionConditionFilter) const {
  HKLGenerator generator(m_crystalStructure.cell(), dMin);

  HKLFilter_const_sptr filter = getDRangeFilter(dMin, dMax);
  if (reflectionConditionFilter) {
    filter = filter & reflectionConditionFilter;
  }

  std::vector<V3D> hkls;
  hkls.reserve(generator.size());

  std::remove_copy_if(generator.begin(), generator.end(),
                      std::back_inserter(hkls), (~filter)->fn());
  return hkls;
}

/// Returns a list of symetrically independent HKLs within the specified
/// d-limits using the default reflection condition filter.
std::vector<V3D> ReflectionGenerator::getUniqueHKLs(double dMin,
                                                    double dMax) const {
  return getUniqueHKLs(dMin, dMax, m_defaultHKLFilter);
}

/// Returns a list of symetrically independent HKLs within the specified
/// d-limits using the specified reflection condition filter.
std::vector<V3D> ReflectionGenerator::getUniqueHKLs(
    double dMin, double dMax,
    HKLFilter_const_sptr reflectionConditionFilter) const {
  HKLGenerator generator(m_crystalStructure.cell(), dMin);

  HKLFilter_const_sptr filter = getDRangeFilter(dMin, dMax);
  if (reflectionConditionFilter) {
    filter = filter & reflectionConditionFilter;
  }

  std::vector<V3D> hkls;
  hkls.reserve(generator.size());

  PointGroup_sptr pg = m_crystalStructure.spaceGroup()->getPointGroup();

  for (auto hkl = generator.begin(); hkl != generator.end(); ++hkl) {
    if (filter->isAllowed(*hkl)) {
      hkls.push_back(pg->getReflectionFamily(*hkl));
    }
  }

  std::sort(hkls.begin(), hkls.end());
  hkls.erase(std::unique(hkls.begin(), hkls.end()), hkls.end());

  return hkls;
}

/// Returns a list of d-values that correspond to the supplied hkl list, using
/// the unit cell of the stored crystal structure.
std::vector<double>
ReflectionGenerator::getDValues(const std::vector<V3D> &hkls) const {
  std::vector<double> dValues;
  dValues.reserve(hkls.size());

  std::transform(hkls.begin(), hkls.end(), std::back_inserter(dValues),
                 LatticeSpacingCalculator(m_crystalStructure.cell()));

  return dValues;
}

/// Returns a list of squared structure factor amplitudes corresponding to the
/// supplied list of HKLs.
std::vector<double>
ReflectionGenerator::getFsSquared(const std::vector<V3D> &hkls) const {
  return m_sfCalculator->getFsSquared(hkls);
}

} // namespace Geometry
} // namespace Mantid
