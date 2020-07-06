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
ImageInfoModel::ImageInfo::ImageInfo(
    ImageInfoModel::ImageInfo::StringItems names)
    : m_names(std::move(names)) {
  m_values.resize(m_names.size(), MissingValue);
}

// ImageInfoModel

/**
 * Appends a name/value to the given list
 */
void ImageInfoModel::addNameAndValue(const std::string &label,
                                     std::vector<QString> &list,
                                     const double value, const int precision,
                                     bool includeValue,
                                     const Mantid::Kernel::Unit_sptr &units) {
  std::wstring unit;
  auto headerLabel = QString::fromStdString(label);
  if (units && !(unit = units->label().utf8()).empty()) {
    headerLabel += "(" + MantidQt::API::toQStringInternal(unit) + ")";
  }
  list.emplace_back(headerLabel);

  if (includeValue) {
    list.emplace_back(QString::number(value, 'f', precision));
  } else
    list.emplace_back(MissingValue);
}

} // namespace MantidWidgets
} // namespace MantidQt
