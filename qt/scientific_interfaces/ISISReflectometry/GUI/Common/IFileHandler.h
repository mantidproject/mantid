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
  virtual ~IFileHandler() {};
  virtual void saveJSONToFile(std::string const &filename, QMap<QString, QVariant> const &map) = 0;
  virtual QMap<QString, QVariant> loadJSONFromFile(std::string const &filename) = 0;
  virtual void saveCSVToFile(std::string const &filename, std::string const &content) const = 0;
  virtual bool fileExists(std::string const &filepath) const = 0;
  virtual std::string getFullFilePath(std::string const &filename) const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
