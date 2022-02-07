// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidJson/DllConfig.h"

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <string>

namespace Mantid::JsonHelpers {

MANTID_JSON_DLL std::string jsonToString(const Json::Value &json, const std::string &indentation = "");
MANTID_JSON_DLL bool parse(const std::string &jsonString, Json::Value *jsonValue, std::string *errors = NULL);
MANTID_JSON_DLL Json::Value stringToJson(const std::string &json);

} // namespace Mantid::JsonHelpers
