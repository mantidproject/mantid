// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_RANGEINLAMBDA_H_
#define MANTID_CUSTOMINTERFACES_RANGEINLAMBDA_H_
#include "Common/DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {

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

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(RangeInLambda const &lhs,
                                               RangeInLambda const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(RangeInLambda const &lhs,
                                               RangeInLambda const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_RANGEINLAMBDA_H_
