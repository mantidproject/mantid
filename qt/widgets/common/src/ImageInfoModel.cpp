// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoModel.h"

#include <iomanip>
#include <sstream>

namespace MantidQt {
namespace MantidWidgets {

void ImageInfoModel::addNameAndValue(const std::string &label,
                                     const double value, const int precision,
                                     std::vector<std::string> &list, bool includeValues) {
  std::ostringstream valueString;
  if (includeValues)
    valueString << std::setprecision(precision) << value;
  else
    valueString << "-";
  list.emplace_back(label);
  list.emplace_back(valueString.str());
}

} // namespace MantidWidgets
} // namespace MantidQt
