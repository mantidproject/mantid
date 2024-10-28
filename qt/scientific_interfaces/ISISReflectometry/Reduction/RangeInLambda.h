// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL RangeInLambda {
public:
  RangeInLambda(double min, double max);

  bool isValid(bool bothOrNoneMustBeSet) const;
  double min() const;
  double max() const;
  bool minSet() const;
  bool maxSet() const;
  bool bothSet() const;
  bool unset() const;

private:
  double m_min, m_max;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(RangeInLambda const &lhs, RangeInLambda const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(RangeInLambda const &lhs, RangeInLambda const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
