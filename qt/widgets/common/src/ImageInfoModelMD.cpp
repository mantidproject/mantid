// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include <climits>

namespace MantidQt::MantidWidgets {

/// @copydoc MantidQt::MantidWidgets::ImageInfoModel::info
ImageInfoModel::ImageInfo ImageInfoModelMD::info(const double x, const double y, const double signal,
                                                 const QMap<QString, QString> &extraValues) const {
  ImageInfo info({"x", "y", "Signal"});
  for (auto const &extraName : extraValues.keys())
    info.m_names.push_back(extraName);

  auto valueOrMissing = [](double value) { return value == UnsetValue ? MissingValue : defaultFormat(value); };
  info.setValue(0, valueOrMissing(x));
  info.setValue(1, valueOrMissing(y));
  info.setValue(2, valueOrMissing(signal));
  for (auto const &extraValue : extraValues.values())
    info.m_values.push_back(extraValue);

  return info;
}

} // namespace MantidQt::MantidWidgets
