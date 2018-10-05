// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RangeInLambda.h"
#include "MantidKernel/Tolerance.h"
namespace MantidQt {
namespace CustomInterfaces {

RangeInLambda::RangeInLambda(double min, double max) : m_min(min), m_max(max) {}

bool RangeInLambda::isValid(bool bothOrNoneMustBeSet) const {
  if (minSet() && maxSet())
    return m_max > m_min + Mantid::Kernel::Tolerance;
  else if (unset())
    return true;
  else
    return !bothOrNoneMustBeSet;
}

double RangeInLambda::min() const { return m_min; }

double RangeInLambda::max() const { return m_max; }

bool RangeInLambda::minSet() const { return m_min > Mantid::Kernel::Tolerance; }

bool RangeInLambda::maxSet() const { return m_max > Mantid::Kernel::Tolerance; }

bool RangeInLambda::bothSet() const { return minSet() && maxSet(); }

bool RangeInLambda::unset() const { return !minSet() && !maxSet(); }

bool operator==(RangeInLambda const &lhs, RangeInLambda const &rhs) {
  return lhs.min() == rhs.min() && lhs.max() == rhs.max();
}

bool operator!=(RangeInLambda const &lhs, RangeInLambda const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
