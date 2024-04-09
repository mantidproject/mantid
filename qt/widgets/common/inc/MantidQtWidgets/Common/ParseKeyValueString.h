// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** parseKeyValueString

Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'`
into a map of key/value pairs.
*/

#include "DllOption.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidKernel/System.h"

#include <QString>
#include <QStringList>
#include <sstream>
#include <string>

#include <map>

namespace MantidQt {
namespace MantidWidgets {

std::map<std::string, std::string> DLLExport parseKeyValueString(const std::string &str,
                                                                 const std::string &separator = ",");
std::map<QString, QString> DLLExport parseKeyValueQString(const QString &str, const std::string &separator = ",");
// Trim leading/trailing whitespace and quotes from a string
void trimWhitespaceAndQuotes(const QString &valueIn);
// Trim whitespace, quotes and empty values from a string list
void trimWhitespaceQuotesAndEmptyValues(QStringList &values);
/// Convert an options map to a string
QString EXPORT_OPT_MANTIDQT_COMMON convertMapToString(const std::map<QString, QString> &optionsMap,
                                                      const char separator = ',', const bool quoteValues = true);
std::string EXPORT_OPT_MANTIDQT_COMMON convertMapToString(const std::map<std::string, std::string> &optionsMap,
                                                          const char separator, const bool quoteValues);
std::string EXPORT_OPT_MANTIDQT_COMMON convertAlgPropsToString(Mantid::API::IAlgorithmRuntimeProps const &options);
std::string EXPORT_OPT_MANTIDQT_COMMON optionsToString(std::map<std::string, std::string> const &options,
                                                       const bool quoteValues = true,
                                                       const std::string &separator = ", ");

QStringList EXPORT_OPT_MANTIDQT_COMMON stdVectorToQStringList(std::vector<std::string> const &vec);
std::vector<std::string> EXPORT_OPT_MANTIDQT_COMMON qStringListToStdVector(QStringList const &qList);
std::vector<std::string> EXPORT_OPT_MANTIDQT_COMMON qListToStdVector(QList<std::string> const &qList);
QVector<QString> EXPORT_OPT_MANTIDQT_COMMON convertStdStringVector(const std::vector<std::string> &stringVec);

} // namespace MantidWidgets
} // namespace MantidQt
