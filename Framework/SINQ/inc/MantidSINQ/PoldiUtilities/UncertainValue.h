// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  */
class MANTID_SINQ_DLL UncertainValue {
public:
  UncertainValue();
  explicit UncertainValue(double value);
  UncertainValue(double value, double error);

  double value() const;
  double error() const;

  operator double() const;
  UncertainValue operator*(double d);
  UncertainValue operator/(double d);
  UncertainValue operator+(double d);
  UncertainValue operator-(double d);

  static const UncertainValue plainAddition(UncertainValue const &left, UncertainValue const &right);

  static bool lessThanError(UncertainValue const &left, UncertainValue const &right);
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
} // namespace Poldi
} // namespace Mantid
