#ifndef UNCERTAINVALUE_H
#define UNCERTAINVALUE_H

#include "MantidSINQ/DllConfig.h"
#include <cmath>
#include <string>

namespace Mantid {
namespace Poldi {

/** UncertainValue :

    A value with error. This class is convenient when numbers with uncertainties
    have to be stored. It supports conversion to double (then only the value
    is supplied, without the uncertainty) and arithmetic operations with
   doubles,
    preserving the errors.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 11/03/2014

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
class MANTID_SINQ_DLL UncertainValue {
public:
  UncertainValue();
  explicit UncertainValue(double value);
  UncertainValue(double value, double error);
  ~UncertainValue() {}

  double value() const;
  double error() const;

  operator double() const;
  UncertainValue operator*(double d);
  UncertainValue operator/(double d);
  UncertainValue operator+(double d);
  UncertainValue operator-(double d);

  static const UncertainValue plainAddition(UncertainValue const &left,
                                            UncertainValue const &right);

  static bool lessThanError(UncertainValue const &left,
                            UncertainValue const &right);
  static double valueToErrorRatio(UncertainValue const &uncertainValue);
  static double errorToValueRatio(UncertainValue const &uncertainValue);

private:
  double m_value;
  double m_error;
};

UncertainValue MANTID_SINQ_DLL operator*(double d, const UncertainValue &v);
UncertainValue MANTID_SINQ_DLL operator/(double d, const UncertainValue &v);
UncertainValue MANTID_SINQ_DLL operator+(double d, const UncertainValue &v);
UncertainValue MANTID_SINQ_DLL operator-(double d, const UncertainValue &v);
}
}

#endif // UNCERTAINVALUE_H
