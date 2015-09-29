#include "MantidGeometry/Crystal/ReflectionGenerator.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/StructureFactorCalculatorSummation.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"

namespace Mantid {
namespace Geometry {

using namespace Kernel;

ReflectionGenerator::ReflectionGenerator(
    const CrystalStructure &crystalStructure,
    ReflectionGenerator::DefaultReflectionConditionFilter defaultFilter)
    : m_crystalStructure(crystalStructure),
      m_sfCalculator(StructureFactorCalculatorFactory::create<
          StructureFactorCalculatorSummation>(m_crystalStructure)),
      m_defaultHKLFilter(getReflectionConditionFilter(defaultFilter)) {}

const CrystalStructure &ReflectionGenerator::getCrystalStructure() const {
  return m_crystalStructure;
}

std::vector<V3D> ReflectionGenerator::getHKLs(double dMin, double dMax) const {
  return getHKLs(dMin, dMax, m_defaultHKLFilter);
}

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

std::vector<V3D> ReflectionGenerator::getUniqueHKLs(double dMin,
                                                    double dMax) const {
  return getUniqueHKLs(dMin, dMax, m_defaultHKLFilter);
}

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

std::vector<double>
ReflectionGenerator::getDValues(const std::vector<V3D> &hkls) const {
  std::vector<double> dValues;
  dValues.reserve(hkls.size());

  std::transform(
      hkls.begin(), hkls.end(), std::back_inserter(dValues),
      std::bind(&ReflectionGenerator::getD, this, std::placeholders::_1));

  return dValues;
}

std::vector<double>
ReflectionGenerator::getFsSquared(const std::vector<V3D> &hkls) const {
  return m_sfCalculator->getFsSquared(hkls);
}

HKLFilter_const_sptr ReflectionGenerator::getDRangeFilter(double dMin,
                                                          double dMax) const {
  return boost::make_shared<const HKLFilterDRange>(m_crystalStructure.cell(),
                                                   dMin, dMax);
}

HKLFilter_const_sptr ReflectionGenerator::getReflectionConditionFilter(
    ReflectionGenerator::DefaultReflectionConditionFilter filter) {
  switch (filter) {
  case DefaultReflectionConditionFilter::Centering:
    return boost::make_shared<const HKLFilterCentering>(
        m_crystalStructure.centering());
    break;
  case DefaultReflectionConditionFilter::SpaceGroup:
    return boost::make_shared<const HKLFilterSpaceGroup>(
        m_crystalStructure.spaceGroup());
    break;
  case DefaultReflectionConditionFilter::StructureFactor:
    return boost::make_shared<const HKLFilterStructureFactor>(m_sfCalculator);
  default:
    return HKLFilter_const_sptr();
  }
}

double ReflectionGenerator::getD(const V3D &hkl) const {
  return m_crystalStructure.cell().d(hkl);
}

} // namespace Geometry
} // namespace Mantid
