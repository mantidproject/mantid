#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <QStringList>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/** Trim matching start/end quotes from the given string
 * @param value : string to trim
 * @param quote : the quote mark to look for
 * @param escape : the character that escapes quotes
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

/** Trim whitespace and quotes from the start/end of a string
 * @param value: the value to trim
 * */
QString trimWhitespaceAndQuotes(const QString &valueIn) {
  // Trim whitespace
  QString value = valueIn.trimmed();

  // Trim double/single quotes
  trimQuotes(value, '"', '\\');
  trimQuotes(value, '\'', '\\');

  // If trimming was done, recurse to remove any nested quotes/whitespace
  if (value.size() > 0 && value != valueIn)
    value = trimWhitespaceAndQuotes(value);

  return value;
}

/**
   Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'`
   into a map of key/value pairs
   @param str The input string
   @throws std::runtime_error on an invalid input string
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
   @param str The input string
   @throws std::runtime_error on an invalid input string
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

/**
   Parses a map into another map, trimming unwanted characters from the
   values
   @param sourceMap The input map
   @returns : the output map
   @throws std::runtime_error on an invalid input value
*/
std::map<QString, QString>
parseKeyValueMap(const std::map<QString, QString> &sourceMap) {
  std::map<QString, QString> kvp;

  for (const auto &it : sourceMap) {
    auto key = trimWhitespaceAndQuotes(it.first);
    auto value = trimWhitespaceAndQuotes(it.second);

    // Trim escape characters
    key.remove("\\");
    value.remove("\\");

    if (key.isEmpty() || value.isEmpty())
      throw std::runtime_error("Invalid key value pair, '" + key.toStdString() +
                               "=" + value.toStdString() + "'");

    kvp[key] = value;
  }

  return kvp;
}
}
}
