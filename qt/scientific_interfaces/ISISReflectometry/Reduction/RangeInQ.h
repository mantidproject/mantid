// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#define MANTID_CUSTOMINTERFACES_RANGEINQ_H_
#include "Common/DllConfig.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RangeInQ {
public:
  RangeInQ(boost::optional<double> min = boost::none,
           boost::optional<double> step = boost::none,
           boost::optional<double> max = boost::none);

  boost::optional<double> min() const;
  boost::optional<double> max() const;
  boost::optional<double> step() const;

private:
  boost::optional<double> m_min;
  boost::optional<double> m_step;
  boost::optional<double> m_max;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(RangeInQ const &lhs,
                                               RangeInQ const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_RANGEINQ_H_
