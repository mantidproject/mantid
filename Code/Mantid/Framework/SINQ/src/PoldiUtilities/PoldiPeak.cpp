#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"

#include <cmath>
#include <stdexcept>
#include "boost/bind.hpp"

namespace Mantid {
namespace Poldi {

PoldiPeak_sptr PoldiPeak::clone() const {
  return PoldiPeak_sptr(new PoldiPeak(*this));
}

const MillerIndices &PoldiPeak::hkl() const { return m_hkl; }

void PoldiPeak::setHKL(MillerIndices hkl) { m_hkl = hkl; }

UncertainValue PoldiPeak::d() const { return m_d; }

UncertainValue PoldiPeak::q() const { return m_q; }

double PoldiPeak::twoTheta(double lambda) const {
  return 2.0 * asin(lambda / (2.0 * m_d));
}

UncertainValue PoldiPeak::fwhm(FwhmRelation relation) const {
  switch (relation) {
  case AbsoluteQ:
    return m_q.value() * m_fwhmRelative;
  case AbsoluteD:
    return m_d.value() * m_fwhmRelative;
  default:
    return m_fwhmRelative;
  }
}

UncertainValue PoldiPeak::intensity() const { return m_intensity; }

void PoldiPeak::setD(UncertainValue d) {
  if (d <= 0.0) {
    throw std::domain_error("d-Value cannot be 0 or smaller.");
  }

  m_d = d;
  m_q = PoldiPeak::dToQ(m_d);
}

void PoldiPeak::setQ(UncertainValue q) {
  if (q <= 0.0) {
    throw std::domain_error("q-Value cannot be 0 or smaller.");
  }

  m_q = q;
  m_d = PoldiPeak::qToD(m_q);
}

void PoldiPeak::setIntensity(UncertainValue intensity) {
  m_intensity = intensity;
}

void PoldiPeak::setFwhm(UncertainValue fwhm, FwhmRelation relation) {
  switch (relation) {
  case AbsoluteQ:
    if (m_q.value() <= 0) {
      throw std::domain_error(
          "Cannot store FWHM for peak with Q-Value less or equal to 0.");
    }

    m_fwhmRelative = fwhm / m_q.value();
    break;
  case AbsoluteD:
    if (m_d.value() <= 0) {
      throw std::domain_error(
          "Cannot store FWHM for peak with d-Value less or equal to 0.");
    }

    m_fwhmRelative = fwhm / m_d.value();
    break;
  default:
    m_fwhmRelative = fwhm;
    break;
  }
}

void PoldiPeak::multiplyErrors(double factor) {
  setQ(UncertainValue(m_q.value(), m_q.error() * factor));
  setFwhm(
      UncertainValue(m_fwhmRelative.value(), m_fwhmRelative.error() * factor),
      PoldiPeak::Relative);
  setIntensity(
      UncertainValue(m_intensity.value(), m_intensity.error() * factor));
}

UncertainValue PoldiPeak::dToQ(UncertainValue d) { return 2.0 * M_PI / d; }

UncertainValue PoldiPeak::qToD(UncertainValue q) { return 2.0 * M_PI / q; }

PoldiPeak_sptr PoldiPeak::create(UncertainValue qValue) {
  return PoldiPeak_sptr(new PoldiPeak(PoldiPeak::qToD(qValue)));
}

PoldiPeak_sptr PoldiPeak::create(double qValue) {
  return PoldiPeak::create(UncertainValue(qValue));
}

PoldiPeak_sptr PoldiPeak::create(UncertainValue qValue,
                                 UncertainValue intensity) {
  return PoldiPeak_sptr(new PoldiPeak(PoldiPeak::qToD(qValue), intensity));
}

PoldiPeak_sptr PoldiPeak::create(double qValue, double intensity) {
  return PoldiPeak::create(UncertainValue(qValue), UncertainValue(intensity));
}

PoldiPeak_sptr PoldiPeak::create(MillerIndices hkl, double dValue) {
  return PoldiPeak_sptr(new PoldiPeak(
      UncertainValue(dValue), UncertainValue(0.0), UncertainValue(0.0), hkl));
}

PoldiPeak_sptr PoldiPeak::create(MillerIndices hkl, UncertainValue dValue,
                                 UncertainValue intensity,
                                 UncertainValue fwhmRelative) {
  return PoldiPeak_sptr(new PoldiPeak(dValue, intensity, fwhmRelative, hkl));
}

bool PoldiPeak::greaterThan(const PoldiPeak_sptr &first,
                            const PoldiPeak_sptr &second,
                            UncertainValue (PoldiPeak::*function)() const) {
  return static_cast<double>(
             boost::bind(function, first.get())()) >
         static_cast<double>(
             boost::bind(function, second.get())());
}

bool PoldiPeak::lessThan(const PoldiPeak_sptr &first,
                         const PoldiPeak_sptr &second,
                         UncertainValue (PoldiPeak::*function)() const) {
  return static_cast<double>(
             boost::bind(function, first.get())()) <
         static_cast<double>(
             boost::bind(function, second.get())());
}

PoldiPeak::PoldiPeak(UncertainValue d, UncertainValue intensity,
                     UncertainValue fwhm, MillerIndices hkl)
    : m_hkl(hkl), m_intensity(intensity) {
  setD(d);
  setFwhm(fwhm, Relative);
}
}
}
