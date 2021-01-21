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

/// @copydoc MantidQt::MantidWidgets::ImageInfoModel::info
ImageInfoModel::ImageInfo ImageInfoModelMD::info(const double x, const double y, const double signal) const {
  ImageInfo info({"x", "y", "Signal"});

  auto valueOrMissing = [](double value) { return value == UnsetValue ? MissingValue : defaultFormat(value); };
  info.setValue(0, valueOrMissing(x));
  info.setValue(1, valueOrMissing(y));
  info.setValue(2, valueOrMissing(signal));

  return info;
}

} // namespace MantidWidgets
} // namespace MantidQt
