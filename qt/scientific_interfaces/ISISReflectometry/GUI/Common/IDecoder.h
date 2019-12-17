// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IDECODER_H
#define MANTID_ISISREFLECTOMETRY_IDECODER_H

#include <QMap>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IMainWindowView;

/** @class IDecoder

IDecoder is an interface for decoding the contents of the reflectometry
interface from a map
*/
class IDecoder {
public:
  virtual ~IDecoder(){};
  virtual void decodeBatch(const IMainWindowView *mwv, int batchIndex,
                           const QMap<QString, QVariant> &map) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
