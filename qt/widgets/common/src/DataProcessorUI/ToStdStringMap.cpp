// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtMantidWidgets/DataProcessorUI/ToStdStringMap.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
std::map<std::string, std::string>
    EXPORT_OPT_MANTIDQT_MANTIDWIDGETS toStdStringMap(std::map<QString, QString> const &inMap) {
  std::map<std::string, std::string> out;
  std::transform(inMap.begin(), inMap.end(), std::inserter(out, out.begin()),
                 [](std::pair<QString, QString> const &kvp) -> std::pair<std::string, std::string> {
                   return std::make_pair(kvp.first.toStdString(), kvp.second.toStdString());
                 });
  return out;
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
