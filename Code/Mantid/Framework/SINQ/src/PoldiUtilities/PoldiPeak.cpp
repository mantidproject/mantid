#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"

#include <cmath>
#include <stdexcept>

namespace Mantid {
namespace Poldi {

const MillerIndices &PoldiPeak::hkl() const
{
    return m_hkl;
}

void PoldiPeak::setHKL(MillerIndices hkl)
{
    m_hkl = hkl;
}

UncertainValue PoldiPeak::d() const
{
    return m_d;
}

UncertainValue PoldiPeak::q() const
{
    return m_q;
}

double PoldiPeak::twoTheta(double lambda) const
{
    return 2.0 * asin(lambda / (2.0 * m_d));
}

UncertainValue PoldiPeak::fwhm() const
{
    return m_fwhm;
}

UncertainValue PoldiPeak::intensity() const
{
    return m_intensity;
}

void PoldiPeak::setD(UncertainValue d)
{
    if(d <= 0.0) {
        throw std::domain_error("d-Value cannot be 0 or smaller.");
    }

    m_d = d;
    m_q = PoldiPeak::dToQ(m_d);
}

void PoldiPeak::setQ(UncertainValue q)
{
    if(q <= 0.0) {
        throw std::domain_error("q-Value cannot be 0 or smaller.");
    }

    m_q = q;
    m_d = PoldiPeak::qToD(m_q);
}

void PoldiPeak::setIntensity(UncertainValue intensity)
{
    m_intensity = intensity;
}

void PoldiPeak::setFwhm(UncertainValue fwhm)
{
    m_fwhm = fwhm;
}

UncertainValue PoldiPeak::dToQ(UncertainValue d)
{
    return 2.0 * M_PI / d;
}

UncertainValue PoldiPeak::qToD(UncertainValue q)
{
    return 2.0 * M_PI / q;
}

PoldiPeak PoldiPeak::create(UncertainValue qValue)
{
    return PoldiPeak(PoldiPeak::qToD(qValue));
}

PoldiPeak PoldiPeak::create(UncertainValue qValue, UncertainValue intensity)
{
    return PoldiPeak(PoldiPeak::qToD(qValue), intensity);
}

PoldiPeak::PoldiPeak(UncertainValue d, UncertainValue intensity, UncertainValue fwhm, MillerIndices hkl) :
    m_hkl(hkl),
    m_intensity(intensity),
    m_fwhm(fwhm)
{
    setD(d);
}

}
}
