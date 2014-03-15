#ifndef MANTID_SINQ_POLDIPEAK_H
#define MANTID_SINQ_POLDIPEAK_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include "boost/shared_ptr.hpp"

namespace Mantid {
namespace Poldi {

class PoldiPeak;

typedef boost::shared_ptr<PoldiPeak> PoldiPeak_sptr;

class MANTID_SINQ_DLL PoldiPeak
{
public:
    ~PoldiPeak() {}

    const MillerIndices& hkl() const;
    void setHKL(MillerIndices hkl);

    UncertainValue d() const;
    UncertainValue q() const;
    double twoTheta(double lambda) const;

    UncertainValue fwhm() const;
    UncertainValue intensity() const;

    void setD(UncertainValue d);
    void setQ(UncertainValue q);
    void setIntensity(UncertainValue intensity);
    void setFwhm(UncertainValue fwhm);

    static PoldiPeak_sptr create(UncertainValue qValue);
    static PoldiPeak_sptr create(UncertainValue qValue, UncertainValue intensity);

    static bool greaterThan(const PoldiPeak_sptr &first, const PoldiPeak_sptr &second, UncertainValue (PoldiPeak::*function)() const);
    static bool lessThan(const PoldiPeak_sptr &first, const PoldiPeak_sptr &second, UncertainValue (PoldiPeak::*function)() const);

private:
    PoldiPeak(UncertainValue d = UncertainValue(), UncertainValue intensity = UncertainValue(), UncertainValue fwhm = UncertainValue(), MillerIndices hkl = MillerIndices());

    static UncertainValue dToQ(UncertainValue d);
    static UncertainValue qToD(UncertainValue q);

    MillerIndices m_hkl;

    UncertainValue m_d;
    UncertainValue m_q;
    UncertainValue m_intensity;
    UncertainValue m_fwhm;
};

}
}

#endif // MANTID_SINQ_POLDIPEAK_H
