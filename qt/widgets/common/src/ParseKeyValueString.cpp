// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <QStringList>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/** Trim matching start/end quotes from the given string
 *
 * @param value [inout] : string to trim
 * @param quote [in] : the quote mark to look for
 * @param escape [in] : the character that escapes quotes
 */
void trimQuotes(QString &value, const char quote, const char escape) {

  // If size is less than 2, nothing to trim (we must have matching
  // quotes at start/end)
  if (value.size() < 2)
    return;

  // Must have matching start/end quotes to trim
  if (value.startsWith(quote) && value.endsWith(quote)) {

    // Check the final quote is not escaped by a backslash (the
    // first character can't be an escape as we've already checked
    // it's a quote mark)
    if (value.size() < 3 || value.at(value.size() - 2) != escape) {
      // Ok, trim it
      value.remove(0, 1);
      value.remove(value.size() - 1, 1);
    }
  }
}

/** Trim whitespace and quotes from the start/end of a string.
 * Edits the value in-place.
 *
 * @param value [inout]: the value to trim
 * */
void trimWhitespaceAndQuotes(QString &value) {
  // Remember original value
  const auto valueIn = value;

  // Trim whitespace
  value = value.trimmed();

  // Trim double/single quotes
  trimQuotes(value, '"', '\\');
  trimQuotes(value, '\'', '\\');

  // If trimming was done, recurse to remove any nested quotes/whitespace
  if (value.size() > 0 && value != valueIn)
    trimWhitespaceAndQuotes(value);
}

/** Trim whitespace and quotes from the start/end for all strings in the
 * given list, and subsequently remove any empty strings from the list.
 * Edits the list in-place.
 * @param values : the list of values to trim
 */
void trimWhitespaceQuotesAndEmptyValues(QStringList &values) {
  for (auto &value : values)
    trimWhitespaceAndQuotes(value);

  values.removeAll("");
}

/**
   Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'`
   into a map of key/value pairs
   @param str The input string
   @throws std::runtime_error on an invalid input string
   @returns : a map of key/value pairs as strings
*/
std::map<std::string, std::string> parseKeyValueString(const std::string &str) {
  /*
    This is a bad example of using a tokenizer, and
    Mantid::Kernel::StringTokenizer should
    ideally be used for this (see LoadProcessedNexus, Fit1D or others for
    examples)

    The reason we must use boost::tokenizer here is that passing a list of
    separators is not
    yet possible with Mantid::Kernel::StringTokenizer.
  */
  boost::tokenizer<boost::escaped_list_separator<char>> tok(
      str, boost::escaped_list_separator<char>("\\", ",", "\"'"));
  std::map<std::string, std::string> kvp;

  for (const auto &it : tok) {
    std::vector<std::string> valVec;
    boost::split(valVec, it, boost::is_any_of("="));

    if (valVec.size() > 1) {
      // We split on all '='s. The first delimits the key, the rest are assumed
      // to be part of the value
      std::string key = valVec[0];
      // Drop the key from the values vector
      valVec.begin() = valVec.erase(valVec.begin());
      // Join the remaining sections,
      std::string value = boost::algorithm::join(valVec, "=");

      // Remove any unwanted whitespace
      boost::trim(key);
      boost::trim(value);

      if (key.empty() || value.empty())
        throw std::runtime_error("Invalid key value pair, '" + it + "'");

      kvp[key] = value;
    } else {
      throw std::runtime_error("Invalid key value pair, '" + it + "'");
    }
  }
  return kvp;
}

/**
   Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'`
   into a map of key/value pairs
   @param qstr The input string
   @throws std::runtime_error on an invalid input string
   @returns : a map of key/value pairs as QStrings
*/
std::map<QString, QString> parseKeyValueQString(const QString &qstr) {
  /*
    This is a bad example of using a tokenizer, and
    Mantid::Kernel::StringTokenizer should
    ideally be used for this (see LoadProcessedNexus, Fit1D or others for
    examples)

    The reason we must use boost::tokenizer here is that passing a list of
    separators is not
    yet possible with Mantid::Kernel::StringTokenizer.
  */
  auto str = qstr.toStdString();
  boost::tokenizer<boost::escaped_list_separator<char>> tok(
      str, boost::escaped_list_separator<char>("\\", ",", "\"'"));
  std::map<QString, QString> kvp;

  for (const auto &it : tok) {
    auto keyValueString = QString::fromStdString(it);
    auto valVec = keyValueString.split("=");

    if (valVec.size() > 1) {
      // We split on all '='s. The first delimits the key, the rest are assumed
      // to be part of the value
      auto key = valVec[0].trimmed();
      // Drop the key from the values vector
      valVec.removeFirst();
      // Join the remaining sections
      auto value = valVec.join("=").trimmed();

      if (key.isEmpty() || value.isEmpty())
        throw std::runtime_error("Invalid key value pair, '" + it + "'");

      kvp[key] = value;
    } else {
      throw std::runtime_error("Invalid key value pair, '" + it + "'");
    }
  }
  return kvp;
}

/** Convert an options map to a comma-separated list of key=value pairs
 */
QString convertMapToString(const std::map<QString, QString> &optionsMap,
                           const char separator, const bool quoteValues) {
  QString result;
  bool first = true;

  for (auto &kvp : optionsMap) {
    if (kvp.second.isEmpty())
      continue;

    if (!first)
      result += separator;
    else
      first = false;

    const auto key = kvp.first;
    auto value = kvp.second;

    if (quoteValues)
      value = "'" + value + "'";

    result += key + "=" + value;
  }

  return result;
}

/** Convert an options map to a comma-separated list of key=value pairs
 */
std::string
convertMapToString(const std::map<std::string, std::string> &optionsMap,
                   const char separator, const bool quoteValues) {
  std::string result;
  bool first = true;

  for (auto &kvp : optionsMap) {
    if (kvp.second.empty())
      continue;

    if (!first)
      result += separator;
    else
      first = false;

    const auto key = kvp.first;
    auto value = kvp.second;

    if (quoteValues)
      value = "'" + value + "'";

    result += key + "=" + value;
  }

  return result;
}

std::string optionsToString(std::map<std::string, std::string> const &options) {
  if (!options.empty()) {
    std::ostringstream resultStream;
    auto optionsKvpIt = options.cbegin();

    auto const &firstKvp = (*optionsKvpIt);
    resultStream << firstKvp.first << "='" << firstKvp.second << '\'';

    for (; optionsKvpIt != options.cend(); ++optionsKvpIt) {
      auto kvp = (*optionsKvpIt);
      resultStream << ", " << kvp.first << "='" << kvp.second << '\'';
    }

    return resultStream.str();
  } else {
    return std::string();
  }
}
} // namespace MantidWidgets
} // namespace MantidQt
