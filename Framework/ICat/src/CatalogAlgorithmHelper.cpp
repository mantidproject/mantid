#include "MantidICat/CatalogAlgorithmHelper.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/assign/list_of.hpp>

namespace Mantid {
namespace ICat {
/**
 * Obtain the error message returned by the IDS.
 * @param HTTPStatus     :: The HTTPStatus returned by the IDS.
 * @param responseStream :: The contents of the stream (a JSON stream) returned
 * from the IDS.
 * @returns An appropriate error message for the user if it exists. Otherwise an
 * empty string.
 */
const std::string CatalogAlgorithmHelper::getIDSError(
    Poco::Net::HTTPResponse::HTTPStatus &HTTPStatus,
    std::istream &responseStream) {
  std::set<Poco::Net::HTTPResponse::HTTPStatus> successHTTPStatus =
      boost::assign::list_of(Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK)(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_CREATED)(
          Poco::Net::HTTPResponse::HTTPStatus::HTTP_ACCEPTED);

  // Cancel the algorithm and output message if status returned
  // from the server is not in our successHTTPStatus set.
  if (successHTTPStatus.find(HTTPStatus) == successHTTPStatus.end()) {
    // Stores the contents of `jsonResponseData` as a json property tree.
    boost::property_tree::ptree json;
    // Convert the stream to a JSON tree.
    boost::property_tree::read_json(responseStream, json);
    // Return the message returned by the server.
    return json.get<std::string>("code") + ": " +
           json.get<std::string>("message");
  }
  // No error occurred, so return an empty string for verification.
  return "";
}
}
}
