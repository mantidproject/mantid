// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/Json.h"
#include <boost/algorithm/string/erase.hpp>
#include <sstream>

namespace Mantid {
namespace Kernel {
namespace JsonHelpers {

/**
 * @brief Return a string given the json value passed, this function handles errors by throwing instead of returning
 * false and writing to a string the error.
 *
 * @param json the json to be converted to a string.
 * @param indentation the indentation can be anything, if nothing is passed then indentations and newline charecters
 * will be ignored. Defaults to nothing.
 * @return std::string
 */
std::string jsonToString(const Json::Value &json, const std::string &indentation) {
  if (!json.isObject()) {
    throw std::invalid_argument("Expected Json::Value of type object or string, found single non-string value type.");
  }

  Json::StreamWriterBuilder builder;
  builder.settings_["indentation"] = indentation;
  auto string = Json::writeString(builder, json);
  return string;
}

/**
 * @brief Return a Json::Value given the string passed in
 *
 * @param json A string that can be turned into valid Json value.
 * @return Json::Value
 */
Json::Value stringToJson(const std::string &json) {
  std::stringstream sstr;
  sstr.str(json);
  Json::Value jsonValue;
  sstr >> jsonValue;
  return jsonValue;
}

/**
 * @brief A wrapper for the parse method used to read in a std::string as a Json::Value, this function shouldn't
 * throw an exception but return false when it fails to parse the std::string.
 *
 * @param jsonString The string that is going to be parsed into a Json::Value
 * @param jsonValue InOut variable that has the Json::Value emplaced into it
 * @param errors InOut varable that has the error
 * @return true on successful parse
 * @return false on failed parse
 */
bool parse(const std::string &jsonString, Json::Value *jsonValue, std::string *errors) {
  ::Json::CharReaderBuilder readerBuilder;
  auto reader = std::unique_ptr<::Json::CharReader>(readerBuilder.newCharReader());

  return reader->parse(jsonString.c_str(), jsonString.c_str() + jsonString.length(), jsonValue, errors);
}

} // namespace JsonHelpers
} // namespace Kernel
} // namespace Mantid