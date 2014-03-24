#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"

#include <cmath>
#include <stdexcept>
#include "boost/bind.hpp"

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

void PoldiPeak::multiplyErrors(double factor)
{
    setQ(UncertainValue(m_q.value(), m_q.error() * factor));
    setFwhm(UncertainValue(m_fwhm.value(), m_fwhm.error() * factor));
    setIntensity(UncertainValue(m_intensity.value(), m_intensity.error() * factor));
}

UncertainValue PoldiPeak::dToQ(UncertainValue d)
{
    return 2.0 * M_PI / d;
}

UncertainValue PoldiPeak::qToD(UncertainValue q)
{
    return 2.0 * M_PI / q;
}

PoldiPeak_sptr PoldiPeak::create(UncertainValue qValue)
{
    return PoldiPeak_sptr(new PoldiPeak(PoldiPeak::qToD(qValue)));
}

PoldiPeak_sptr PoldiPeak::create(UncertainValue qValue, UncertainValue intensity)
{
    return PoldiPeak_sptr(new PoldiPeak(qValue, intensity));
}

PoldiPeak_sptr PoldiPeak::create(MillerIndices hkl, UncertainValue dValue, UncertainValue intensity, UncertainValue fwhm)
{
    return PoldiPeak_sptr(new PoldiPeak(dValue, intensity, fwhm, hkl));
}

bool PoldiPeak::greaterThan(const PoldiPeak_sptr &first, const PoldiPeak_sptr &second, UncertainValue (PoldiPeak::*function)() const)
{
    return static_cast<double>(boost::bind<UncertainValue>(function, first.get())()) > static_cast<double>(boost::bind<UncertainValue>(function, second.get())());
}

bool PoldiPeak::lessThan(const PoldiPeak_sptr &first, const PoldiPeak_sptr &second, UncertainValue (PoldiPeak::*function)() const)
{
    return static_cast<double>(boost::bind<UncertainValue>(function, first.get())()) < static_cast<double>(boost::bind<UncertainValue>(function, second.get())());
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
