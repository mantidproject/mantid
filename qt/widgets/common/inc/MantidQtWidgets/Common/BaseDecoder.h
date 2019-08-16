// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_ABSTRACTDECODER_H_
#define MANTIDQT_API_ABSTRACTDECODER_H_

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace API {

class BaseDecoder {
public:
  virtual void decode(const QMap<QString, QVariant> &map) = 0;
  virtual QList<QString> tags() = 0;
  virtual ~BaseDecoder() = default;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_ABSTRACTDECODER_H_ */