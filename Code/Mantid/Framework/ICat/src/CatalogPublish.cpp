#include "MantidICat/CatalogPublish.h"
#include "MantidICat/CatalogAlgorithmHelper.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PropertyWithValue.h"

#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include <iostream>
#include <fstream>

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogPublish)

    /// Sets documentation strings for this algorithm
    void CatalogPublish::initDocs()
    {
      this->setWikiSummary("Allows the user to publish data to the catalog.");
    }

    /// Init method to declare algorithm properties
    void CatalogPublish::init()
    {
      declareProperty(new Mantid::API::FileProperty("Filepath", "", Mantid::API::FileProperty::Load), "The file to publish.");
      declareProperty("CreateFileName","","The name to give to the file being saved");
    }

    /// Execute the algorithm
    void CatalogPublish::exec()
    {
      try
      {
        std::string filePath       = getPropertyValue("Filepath");
        std::string createFileName = getPropertyValue("CreateFileName");

        // Extracts the file name (e.g. CSP74683_ICPevent) from the file path.
        std::string dataFileName = Poco::Path(Poco::Path(filePath).getFileName()).getBaseName();
        // Extracts the specific file name (e.g. CSP74683) from the file path.
        dataFileName = dataFileName.substr(0, dataFileName.find_first_of('_'));

        // Create a catalog & obtain the url to PUT (publish) the file to.
        std::string url = CatalogAlgorithmHelper().createCatalog()->getUploadURL(dataFileName, createFileName);
        Poco::URI uri(url);
        std::string path(uri.getPathAndQuery());

        // Currently do not use any means of authentication. This should be updated IDS has signed certificate.
        const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE);
        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);

        // Send the HTTP request, and obtain the output stream to write to. E.g. the data to publish to the server.
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_PUT, path, Poco::Net::HTTPMessage::HTTP_1_1);
        std::ostream& os = session.sendRequest(request);

        // Stream the contents of the file the user wants to publish & store it in file.
        std::ifstream file(filePath.c_str());
        Poco::StreamCopier::copyStream(file, os);
        
        // Close the request by requesting a response.
        Poco::Net::HTTPResponse response;
        session.receiveResponse(response);
      }
      catch(Poco::Net::SSLException& error)
      {
        throw std::runtime_error(error.displayText());
      }
      catch(Poco::Exception& e) {}
    }
  }
}
