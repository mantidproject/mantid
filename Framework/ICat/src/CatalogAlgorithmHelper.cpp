// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidICat/CatalogAlgorithmHelper.h"

#include <json/reader.h>
#include <json/value.h>
#include <set>

using Poco::Net::HTTPResponse;

namespace Mantid::ICat {

/**
 * Obtain the error message returned by the IDS.
 * @param HTTPStatus     :: The HTTPStatus returned by the IDS.
 * @param responseStream :: The contents of the stream (a JSON stream) returned
 * from the IDS.
 * @returns An appropriate error message for the user if it exists. Otherwise an
 * empty string.
 */
const std::string CatalogAlgorithmHelper::getIDSError(const HTTPResponse::HTTPStatus &HTTPStatus,
                                                      std::istream &responseStream) {
  std::set<HTTPResponse::HTTPStatus> successHTTPStatus = {HTTPResponse::HTTP_OK, HTTPResponse::HTTP_CREATED,
                                                          HTTPResponse::HTTP_ACCEPTED};

  // HTTP Status is not one of the positive statuses
  if (successHTTPStatus.find(HTTPStatus) == successHTTPStatus.end()) {
    // Attempt to parse response as json stream
    Json::Value json;
    ::Json::CharReaderBuilder readerBuilder;
    std::string errors;

    // Error messages from IDS are returned as json
    if (Json::parseFromStream(readerBuilder, responseStream, &json, &errors)) {
      return json.get("code", "UNKNOWN").asString() + ": " + json.get("message", "Unknown Error").asString();
    } else {
      // Sometimes the HTTP server can throw an error (which is plain HTML)
      return "HTTP Error: " + std::to_string(HTTPStatus);
    }
  }

  // No error occurred, so return an empty string for verification.
  return "";
}

} // namespace Mantid::ICat
