#include "MantidSINQ/PoldiUtilities/MillerIndices.h"

#include <stdexcept>
#include "boost/format.hpp"

namespace Mantid {
namespace Poldi {

MillerIndices::MillerIndices(int h, int k, int l) :
    m_h(h),
    m_k(k),
    m_l(l),
    m_asVector(3)
{
    populateVector();
}

MillerIndices::MillerIndices(std::vector<int> hkl)
{
    if(hkl.size() != 3) {
        throw std::runtime_error("MillerIndices object can only be created with 3 indices");
    }

    m_asVector = hkl;
    m_h = hkl[0];
    m_k = hkl[1];
    m_l = hkl[2];
}

int MillerIndices::h() const
{
    return m_h;
}

int MillerIndices::k() const
{
    return m_k;
}

int MillerIndices::l() const
{
    return m_l;
}

int MillerIndices::operator [](int index)
{
    if(index < 0 || index > 2) {
        throw std::range_error("Index for accessing hkl is out of range.");
    }

    return m_asVector[index];
}

const std::vector<int> &MillerIndices::asVector() const
{
    return m_asVector;
}

void MillerIndices::populateVector()
{
    m_asVector[0] = m_h;
    m_asVector[1] = m_k;
    m_asVector[2] = m_l;
}

}
}

