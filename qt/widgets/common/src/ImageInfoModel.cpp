// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoModel.h"
#include "MantidQtWidgets/Common/QStringUtils.h"

#include <sstream>

namespace {}

namespace MantidQt {
namespace MantidWidgets {

// ImageInfo

/**
 * Construct an ImageInfo to store name/value pairs
 * @param names The names of the name/value pairs in the table
 */
ImageInfoModel::ImageInfo::ImageInfo(ImageInfoModel::ImageInfo::StringItems names) : m_names(std::move(names)) {
  m_values.resize(m_names.size(), MissingValue);
}

} // namespace MantidWidgets
} // namespace MantidQt
