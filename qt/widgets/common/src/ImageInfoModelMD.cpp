// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include <climits>

namespace MantidQt {
namespace MantidWidgets {

std::vector<QString> ImageInfoModelMD::getInfoList(const double x,
                                                   const double y,
                                                   const double signal) {
  std::vector<QString> list;
  constexpr auto dblmax(std::numeric_limits<double>::max());
  addNameAndValue("x", list, x, 4, (x != dblmax));
  addNameAndValue("y", list, y, 4, (y != dblmax));
  addNameAndValue("Signal", list, signal, 4, (signal != dblmax));

  return list;
}

} // namespace MantidWidgets
} // namespace MantidQt
