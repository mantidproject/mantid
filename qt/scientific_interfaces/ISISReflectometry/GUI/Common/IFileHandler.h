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
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
/** @class IFileHandler

IFileHandler is an interface for saving/loading files
*/
class IFileHandler {
public:
  virtual ~IFileHandler(){};
  virtual void saveJSONToFile(std::string const &filename, QMap<QString, QVariant> const &map) = 0;
  virtual QMap<QString, QVariant> loadJSONFromFile(std::string const &filename) = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt