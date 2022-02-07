// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QWidget>

namespace MantidQt {
namespace API {

class BaseDecoder {
public:
  virtual QWidget *decode(const QMap<QString, QVariant> &map, const std::string &directory) = 0;
  virtual QList<QString> tags() = 0;
  virtual ~BaseDecoder() = default;
};

} // namespace API
} // namespace MantidQt
