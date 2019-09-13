// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_ABSTRACTENCODER_H_
#define MANTIDQT_API_ABSTRACTENCODER_H_

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace API {

class BaseEncoder {
public:
  virtual QMap<QString, QVariant> encode(const QWidget *window,
                                         const std::string &directory) = 0;
  virtual QList<QString> tags() = 0;
  virtual ~BaseEncoder() = default;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_ABSTRACTENCODER_H_ */