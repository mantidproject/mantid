// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_PARSEKEYVALUESTRING_H
#define MANTIDQTMANTIDWIDGETS_PARSEKEYVALUESTRING_H

/** parseKeyValueString

Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'`
into a map of key/value pairs.
*/

#include "DllOption.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include <QString>
#include <map>
#include <sstream>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

std::map<std::string, std::string>
    DLLExport parseKeyValueString(const std::string &str);
MantidQt::MantidWidgets::DataProcessor::OptionsMap DLLExport
parseKeyValueQString(const QString &str);
// Trim leading/trailing whitespace and quotes from a string
void trimWhitespaceAndQuotes(const QString &valueIn);
// Trim whitespace, quotes and empty values from a string list
void trimWhitespaceQuotesAndEmptyValues(QStringList &values);
/// Convert an options map to a string
QString EXPORT_OPT_MANTIDQT_COMMON
convertMapToString(const std::map<QString, QString> &optionsMap,
                   const char separator = ',', const bool quoteValues = true);
std::string EXPORT_OPT_MANTIDQT_COMMON
convertMapToString(const std::map<std::string, std::string> &optionsMap,
                   const char separator, const bool quoteValues);
std::string EXPORT_OPT_MANTIDQT_COMMON
optionsToString(std::map<std::string, std::string> const &options);
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_PARSEKEYVALUESTRING_H
