// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>
#include <map>
#include <string>
/**

This file defines functions for converting a QString -> QString map to a
std::string -> std::string map.
*/
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
std::map<std::string, std::string> EXPORT_OPT_MANTIDQT_COMMON toStdStringMap(std::map<QString, QString> const &inMap);
}
} // namespace MantidWidgets
} // namespace MantidQt