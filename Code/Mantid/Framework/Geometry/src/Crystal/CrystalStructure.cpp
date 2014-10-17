#include "MantidGeometry/Crystal/CrystalStructure.h"
#include <boost/bind.hpp>
#include <stdexcept>

namespace Mantid
{
namespace Geometry
{

using namespace Mantid::Kernel;

/// Constructor - only unit cell is required, pointgroup has default -1, centering P
CrystalStructure::CrystalStructure(const UnitCell &unitCell, const PointGroup_sptr &pointGroup, const ReflectionCondition_sptr &centering) :
    m_cell(unitCell),
    m_pointGroup(pointGroup),
    m_centering(centering)
{
}

/// Returns the unit cell of the structure
UnitCell CrystalStructure::cell() const
{
    return m_cell;
}

/// Assigns a new unit cell
void CrystalStructure::setCell(const UnitCell &cell)
{
    m_cell = cell;
}

/// Returns the structure's point group
PointGroup_sptr CrystalStructure::pointGroup() const
{
    return m_pointGroup;
}

/// Assigns a new point group
void CrystalStructure::setPointGroup(const PointGroup_sptr &pointGroup)
{
    m_pointGroup = pointGroup;
}

/// Convenience method to get the PointGroup's crystal system
PointGroup::CrystalSystem CrystalStructure::crystalSystem() const
{
    if(!m_pointGroup) {
        throw std::invalid_argument("Cannot determine crystal system from null-PointGroup.");
    }

    return m_pointGroup->crystalSystem();
}

/// Returns the centering of the structure
ReflectionCondition_sptr CrystalStructure::centering() const
{
    return m_centering;
}

/// Assigns a new centering.
void CrystalStructure::setCentering(const ReflectionCondition_sptr &centering)
{
    m_centering = centering;
}

/// Returns a vector with all allowed HKLs in the given d-range
std::vector<Kernel::V3D> CrystalStructure::getHKLs(double dMin, double dMax) const
{
    if(!m_centering) {
        throw std::invalid_argument("Insufficient data for creation of a reflection list, need at least centering and unit cell.");
    }

    if(dMin <= 0.0) {
        throw std::invalid_argument("dMin is <= 0.0, not a valid spacing.");
    }

    if(dMax <= dMin) {
        throw std::invalid_argument("dMax must be larger than dMin.");
    }

    std::vector<V3D> hkls;

    /* There's an estimation for the number of reflections from the unit cell volume
     * and the minimum d-value. This should speed up insertion into the vector.
     */
    size_t estimatedReflectionCount = static_cast<size_t>( ceil(32.0 * M_PI * m_cell.volume()) / (3.0 * pow(2.0 * dMin, 3.0)) );
    hkls.reserve(estimatedReflectionCount);

    int hMax = static_cast<int>(m_cell.a() / dMin);
    int kMax = static_cast<int>(m_cell.b() / dMin);
    int lMax = static_cast<int>(m_cell.c() / dMin);

    for(int h = -hMax; h <= hMax; ++h) {
        for(int k = -kMax; k <= kMax; ++k) {
            for(int l = -lMax; l <= lMax; ++l) {
                if(m_centering->isAllowed(h, k, l)) {
                    V3D hkl(h, k, l);
                    double d = m_cell.d(hkl);

                    if(d <= dMax && d >= dMin) {
                        hkls.push_back(hkl);
                    }
                }
            }
        }
    }

    return hkls;
}

/// Returns a vector with all allowed symmetry independent HKLs (depends on point group) in the given d-range
std::vector<Kernel::V3D> CrystalStructure::getUniqueHKLs(double dMin, double dMax) const
{
    if(!m_centering || !m_pointGroup) {
        throw std::invalid_argument("Insufficient data for creation of a reflection list, need at least centering, unit cell and point group.");
    }

    if(dMin <= 0.0) {
        throw std::invalid_argument("dMin is <= 0.0, not a valid spacing.");
    }

    if(dMax <= dMin) {
        throw std::invalid_argument("dMax must be larger than dMin.");
    }

    std::set<V3D> uniqueHKLs;

    int hMax = static_cast<int>(m_cell.a() / dMin);
    int kMax = static_cast<int>(m_cell.b() / dMin);
    int lMax = static_cast<int>(m_cell.c() / dMin);

    for(int h = -hMax; h <= hMax; ++h) {
        for(int k = -kMax; k <= kMax; ++k) {
            for(int l = -lMax; l <= lMax; ++l) {
                if(m_centering->isAllowed(h, k, l)) {
                    V3D hkl(h, k, l);
                    double d = m_cell.d(hkl);

                    if(d <= dMax && d >= dMin) {
                        V3D uniqueHKL = m_pointGroup->getReflectionFamily(hkl);

                        uniqueHKLs.insert(uniqueHKL);
                    }
                }
            }
        }
    }

    return std::vector<V3D>(uniqueHKLs.begin(), uniqueHKLs.end());
}

/// Maps a vector of hkls to d-values using the internal cell
std::vector<double> CrystalStructure::getDValues(const std::vector<V3D> &hkls) const
{

    std::vector<double> dValues(hkls.size());
    std::transform(hkls.begin(), hkls.end(), dValues.begin(), boost::bind<double>(&UnitCell::d, m_cell, _1));

    return dValues;
}


} // namespace Geometry
} // namespace Mantid
