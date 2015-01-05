#ifndef MANTID_SINQ_POLDIPEAK_H
#define MANTID_SINQ_POLDIPEAK_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "MantidSINQ/PoldiUtilities/MillerIndices.h"
#include "boost/shared_ptr.hpp"

namespace Mantid {
namespace Poldi {
/** PoldiPeak :

    Representation of reflection peaks, probably not only within the
    scope of a POLDI experiment, but so far limited to it.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 14/03/2014

    Copyright © 2014 PSI-MSS

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

class PoldiPeak;

typedef boost::shared_ptr<PoldiPeak> PoldiPeak_sptr;

class MANTID_SINQ_DLL PoldiPeak {
public:
  enum FwhmRelation { AbsoluteQ, AbsoluteD, Relative };

  ~PoldiPeak() {}

  PoldiPeak_sptr clone() const;

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
  static PoldiPeak_sptr create(MillerIndices hkl, UncertainValue dValue,
                               UncertainValue intensity,
                               UncertainValue fwhmRelative);

  static bool greaterThan(const PoldiPeak_sptr &first,
                          const PoldiPeak_sptr &second,
                          UncertainValue (PoldiPeak::*function)() const);
  static bool lessThan(const PoldiPeak_sptr &first,
                       const PoldiPeak_sptr &second,
                       UncertainValue (PoldiPeak::*function)() const);

private:
  PoldiPeak(UncertainValue d = UncertainValue(),
            UncertainValue intensity = UncertainValue(),
            UncertainValue fwhm = UncertainValue(),
            MillerIndices hkl = MillerIndices());

  static UncertainValue dToQ(UncertainValue d);
  static UncertainValue qToD(UncertainValue q);

  MillerIndices m_hkl;

  UncertainValue m_d;
  UncertainValue m_q;
  UncertainValue m_intensity;
  UncertainValue m_fwhmRelative;
};
}
}

#endif // MANTID_SINQ_POLDIPEAK_H
