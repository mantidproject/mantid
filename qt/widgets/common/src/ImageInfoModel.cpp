// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoModel.h"
#include "MantidQtWidgets/Common/QStringUtils.h"

#include <QRegExp>
#include <iomanip>
#include <sstream>

namespace MantidQt {
namespace MantidWidgets {

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
    QString valueString = QString::number(value, 'f', precision);
    // remove any trailing zeros after decimal place
    valueString.replace(QRegExp("(\\.\\d*[1-9])(0+)$|\\.0+$"), "\\1");
    list.emplace_back(valueString);
  } else
    list.emplace_back(QString("-"));
}

} // namespace MantidWidgets
} // namespace MantidQt
