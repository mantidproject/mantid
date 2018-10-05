// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#define MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RangeInQ {
public:
  RangeInQ(double min, double step, double max);
  double min() const;
  double max() const;
  double step() const;

private:
  double m_min;
  double m_step;
  double m_max;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_RANGEINQ_H_
