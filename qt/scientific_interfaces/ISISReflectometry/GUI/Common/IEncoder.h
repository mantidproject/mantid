// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QMap>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IMainWindowView;

/** @class IEncoder

IEncoder is an interface for encoding the contents of the reflectometry
interface into a map
*/
class IEncoder {
public:
  virtual ~IEncoder(){};
  virtual QMap<QString, QVariant> encodeBatch(const IMainWindowView *mwv, int batchIndex, bool projectSave = false) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
