// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "boost/shared_ptr.hpp"

namespace Mantid {
namespace Poldi {
/** PoldiPeak :

    Representation of reflection peaks, probably not only within the
    scope of a POLDI experiment, but so far limited to it.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 14/03/2014
  */

class PoldiPeak;

using PoldiPeak_sptr = std::shared_ptr<PoldiPeak>;

class MANTID_SINQ_DLL PoldiPeak {
public:
  enum FwhmRelation { AbsoluteQ, AbsoluteD, Relative };

  PoldiPeak_sptr clonePeak() const;

  const MillerIndices &hkl() const;
  void setHKL(MillerIndices hkl);

  UncertainValue d() const;
  UncertainValue q() const;
  double twoTheta(double lambda) const;

  UncertainValue fwhm(FwhmRelation relation = AbsoluteQ) const;
  UncertainValue intensity() const;

  void setD(UncertainValue d);
  void setQ(UncertainValue q);
  void setIntensity(UncertainValue intensity);
  void setFwhm(UncertainValue fwhm, FwhmRelation relation = AbsoluteQ);

  void multiplyErrors(double factor);

  static PoldiPeak_sptr create(UncertainValue qValue);
  static PoldiPeak_sptr create(double qValue);
  static PoldiPeak_sptr create(UncertainValue qValue, UncertainValue intensity);
  static PoldiPeak_sptr create(double qValue, double intensity);
  static PoldiPeak_sptr create(MillerIndices hkl, double dValue);
  static PoldiPeak_sptr create(MillerIndices hkl, UncertainValue dValue, UncertainValue intensity,
                               UncertainValue fwhmRelative);

  static bool greaterThan(const PoldiPeak_sptr &first, const PoldiPeak_sptr &second,
                          UncertainValue (PoldiPeak::*function)() const);
  static bool lessThan(const PoldiPeak_sptr &first, const PoldiPeak_sptr &second,
                       UncertainValue (PoldiPeak::*function)() const);

private:
  PoldiPeak(UncertainValue d = UncertainValue(), UncertainValue intensity = UncertainValue(),
            UncertainValue fwhm = UncertainValue(), MillerIndices hkl = MillerIndices());

  static UncertainValue dToQ(UncertainValue d);
  static UncertainValue qToD(UncertainValue q);

  MillerIndices m_hkl;

  UncertainValue m_d;
  UncertainValue m_q;
  UncertainValue m_intensity;
  UncertainValue m_fwhmRelative;
};
} // namespace Poldi
} // namespace Mantid
