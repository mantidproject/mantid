#include "MantidICat/CatalogAlgorithmHelper.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace Mantid
{
  namespace ICat
  {

    /**
     * Create a catalog to use in the algorithms.
     * @return A pointer to the catalog class.
     */
    API::ICatalog_sptr CatalogAlgorithmHelper::createCatalog()
    {
      API::ICatalog_sptr catalog;
      try
      {
        catalog = API::CatalogFactory::Instance().create(Kernel::ConfigService::Instance().getFacility().catalogInfo().catalogName());
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Your current Facility: " + Kernel::ConfigService::Instance().getFacility().name() + " does not have catalog information.\n");
      }
      return catalog;
    }


    /**
     * Obtain the error message returned by the IDS.
     * @param HTTPStatus     :: The HTTPStatus returned by the IDS.
     * @param responseStream :: The contents of the stream (a JSON stream) returned from the IDS.
     * @returns An appropriate error message for the user if it exists. Otherwise an empty string.
     */
    const std::string CatalogAlgorithmHelper::getIDSError(const std::string &HTTPStatus, std::istream& responseStream)
    {
      // Set containing all valid HTTPStatus'.
      std::string tmp[] = {"200", "201", "202"};
      std::set<std::string> successHTTPStatus(tmp, tmp + sizeof(tmp) / sizeof(tmp[0]));

      // Cancel the algorithm and output message if status returned
      // from the server is not in our successHTTPStatus set.
      if (successHTTPStatus.find(HTTPStatus) == successHTTPStatus.end())
      {
        // Stores the contents of `jsonResponseData` as a json property tree.
        boost::property_tree::ptree json;
        // Convert the stream to a JSON tree.
        boost::property_tree::read_json(responseStream, json);
        // Return the message returned by the server.
        return json.get<std::string>("code") + ": " + json.get<std::string>("message");
      }
      // No error occurred, so return an empty string for verification.
      return "";
    }
  }
}
