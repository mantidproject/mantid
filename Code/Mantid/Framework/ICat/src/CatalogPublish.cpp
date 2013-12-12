#include "MantidICat/CatalogPublish.h"
#include "MantidICat/CatalogAlgorithmHelper.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/MandatoryValidator.h"

#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

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
      declareProperty(new Mantid::API::FileProperty("Filepath", "", Mantid::API::FileProperty::OptionalLoad), "The file to publish.");
      declareProperty(new Mantid::API::WorkspaceProperty<Mantid::API::Workspace>(
            "InputWorkspace","", Mantid::Kernel::Direction::Input,Mantid::API::PropertyMode::Optional),"An input workspace to publish.");
      declareProperty("CreateFileName","",boost::make_shared<Kernel::MandatoryValidator<std::string>>(),"The name to give to the file being saved");
    }

    /// Execute the algorithm
    void CatalogPublish::exec()
    {
      Mantid::API::Workspace_sptr workspace = getProperty("InputWorkspace");

      std::string ws             = getPropertyValue("InputWorkspace");
      std::string filePath       = getPropertyValue("Filepath");
      std::string createFileName = getPropertyValue("CreateFileName");

      // Error checking to ensure a workspace OR a file is selected. Never both.
      if ((ws.empty() && filePath.empty()) || (!ws.empty() && !filePath.empty()))
      {
        throw std::runtime_error("Please select a workspace or a file to publish. Not both.");
      }

      std::string dataFileName;

      // The user want to upload a file.
      if (!filePath.empty())
      {
        dataFileName = extractFileName(filePath);
      }
      else
      {
      }

      std::string uploadURL = CatalogAlgorithmHelper().createCatalog()->getUploadURL(dataFileName, createFileName);
      publish(filePath,uploadURL);
    }

    /**
     * Upload a given file (based on file path) to a given URL.
     * @param pathToFileToUpload  :: The path to the file we want to publish.
     * @param uploadURL           :: The REST URL to stream the data from the file to.
     */
    void CatalogPublish::publish(const std::string &pathToFileToUpload, const std::string &uploadURL)
    {
      try
      {
        Poco::URI uri(uploadURL);
        std::string path(uri.getPathAndQuery());

        // Currently do not use any means of authentication. This should be updated IDS has signed certificate.
        const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE);
        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);

        // Send the HTTP request, and obtain the output stream to write to. E.g. the data to publish to the server.
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_PUT, path, Poco::Net::HTTPMessage::HTTP_1_1);
        // Sets the encoding type of the request. This enables us to stream data to the server.
        request.setChunkedTransferEncoding(true);
        std::ostream& os = session.sendRequest(request);

        // Obtain the mode to used base on file extension.
        std::ios_base::openmode mode = isDataFile(pathToFileToUpload) ? std::ios_base::binary : std::ios_base::in;
        // Stream the contents of the file the user wants to publish & store it in file.
        std::ifstream file(pathToFileToUpload.c_str(), mode);

        // Copy data from the input stream to the server (request) output stream.
        Poco::StreamCopier::copyStream(file, os);
        
        // Close the request by requesting a response.
        Poco::Net::HTTPResponse response;
        session.receiveResponse(response);
      }
      catch(Poco::Net::SSLException& error)
      {
        throw std::runtime_error(error.displayText());
      }
      catch(Poco::Exception&) {}
    }

    /**
    * Checks to see if the file to be downloaded is a datafile.
    * @param filePath :: Path of data file to use.
    * @returns True if the file in the path is a data file.
    */
    bool CatalogPublish::isDataFile(const std::string & filePath)
    {
      std::string extension = Poco::Path(filePath).getExtension();
      std::transform(extension.begin(),extension.end(),extension.begin(),tolower);
      return extension.compare("raw") == 0 || extension.compare("nxs") == 0;
    }

    /**
     * Extract the name of the file from a given path.
     * @param filePath :: Path of data file to use.
     * @returns The filename of the given path
     */
    const std::string CatalogPublish::extractFileName(const std::string &filePath)
    {
      // Extracts the file name (e.g. CSP74683_ICPevent) from the file path.
      std::string dataFileName = Poco::Path(Poco::Path(filePath).getFileName()).getBaseName();
      // Extracts the specific file name (e.g. CSP74683) from the file path.
      return dataFileName.substr(0, dataFileName.find_first_of('_'));
    }

  }
}
