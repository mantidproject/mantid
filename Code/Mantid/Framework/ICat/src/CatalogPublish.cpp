#include "MantidICat/CatalogPublish.h"
#include "MantidICat/CatalogAlgorithmHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/MandatoryValidator.h"

#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Path.h>
#include <Poco/SharedPtr.h>
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
      declareProperty(new Mantid::API::FileProperty("FileName", "", Mantid::API::FileProperty::OptionalLoad), "The file to publish.");
      declareProperty(new Mantid::API::WorkspaceProperty<Mantid::API::Workspace>(
            "InputWorkspace","", Mantid::Kernel::Direction::Input,Mantid::API::PropertyMode::Optional),"An input workspace to publish.");
      declareProperty("NameInCatalog","","The name to give to the file being saved. The file name or workspace name is used by default.");
      declareProperty("InvestigationNumber","","The investigation number to save the file to. Extracted from filename or workspace name, but can be overridden.");
    }

    /// Execute the algorithm
    void CatalogPublish::exec()
    {
      // Used for error checking.
      std::string ws       = getPropertyValue("InputWorkspace");
      std::string filePath = getPropertyValue("FileName");
      Mantid::API::Workspace_sptr workspace = getProperty("InputWorkspace");

      // Error checking to ensure a workspace OR a file is selected. Never both.
      if ((ws.empty() && filePath.empty()) || (!ws.empty() && !filePath.empty()))
      {
        throw std::runtime_error("Please select a workspace or a file to publish. Not both.");
      }

      // The name of the file, which is used to obtain the dataset ID in getUploadURL below.
      std::string dataFileName = getPropertyValue("InvestigationNumber");

      // Create a catalog as getUploadURL is called twice if workspace is selected.
      auto catalog = CatalogAlgorithmHelper().createCatalog();

      // The user want to upload a file.
      if (!filePath.empty())
      {
        // If the user has not specified then an investigation number to use then obtain it from the filename.
        if (dataFileName.empty()) dataFileName = extractFileName(filePath);
        // If the user has not set the name to save the file as, then use the filename of the file being uploaded.
        if (getPropertyValue("NameInCatalog").empty()) setProperty("NameInCatalog", Poco::Path(filePath).getFileName());
      }
      else // The user wants to upload a workspace.
      {
        if (dataFileName.empty()) dataFileName = extractFileName(workspace->name());
        if (getPropertyValue("NameInCatalog").empty()) setProperty("NameInCatalog", workspace->name());
        // Save workspace to a .nxs file in the user's default directory.
        saveWorkspaceToNexus(workspace);
        // Overwrite the filePath string to the location of the file (from which the workspace was saved to).
        filePath = Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") + workspace->name() + ".nxs";
      }

      // Obtain the mode to used base on file extension.
      std::ios_base::openmode mode = isDataFile(filePath) ? std::ios_base::binary : std::ios_base::in;
      // Stream the contents of the file the user wants to publish & store it in file.
      std::ifstream fileStream(filePath.c_str(), mode);
      // Verify that the file can be opened correctly.
      if (fileStream.rdstate() & std::ios::failbit) throw Mantid::Kernel::Exception::FileError("Error on opening file at: ", filePath);
      // Publish the contents of the file to the server.

      publish(fileStream,catalog->getUploadURL(dataFileName, getPropertyValue("NameInCatalog")));
      // If a workspace was published, then we want to also publish the history of a workspace.
      if (!ws.empty()) publishWorkspaceHistory(catalog, workspace, dataFileName);
    }

    /**
     * Stream the contents of a file to a given URL.
     * @param fileContents :: The contents of the file to publish.
     * @param uploadURL    :: The REST URL to stream the data from the file to.
     */
    void CatalogPublish::publish(std::istream& fileContents, const std::string &uploadURL)
    {
      try
      {
        Poco::URI uri(uploadURL);
        std::string path(uri.getPathAndQuery());

        Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> certificateHandler = new Poco::Net::AcceptCertificateHandler(true);
        // Currently do not use any means of authentication. This should be updated IDS has signed certificate.
        const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE);
        // Create a singleton for holding the default context. E.g. any future requests to publish are made to this certificate and context.
        Poco::Net::SSLManager::instance().initializeClient(NULL, certificateHandler,context);
        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);

        // Send the HTTP request, and obtain the output stream to write to. E.g. the data to publish to the server.
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_PUT, path, Poco::Net::HTTPMessage::HTTP_1_1);
        // Sets the encoding type of the request. This enables us to stream data to the server.
        request.setChunkedTransferEncoding(true);
        std::ostream& os = session.sendRequest(request);

        // Copy data from the input stream to the server (request) output stream.
        Poco::StreamCopier::copyStream(fileContents, os);
        
        // Close the request by requesting a response.
        Poco::Net::HTTPResponse response;
        session.receiveResponse(response);

        std::string HTTPStatus = boost::lexical_cast<std::string>(response.getStatus());

        // Throw an error if publishing was not successful.
        // (Note: The IDS does not currently return any meta-data related to the errors caused.)
        if (HTTPStatus.find("20") == std::string::npos)
        {
          g_log.error("An error has occurred on the ICAT IDS server.\n"
                      "A file with that name already exists or you do not have permissions to publish to that investigation.");
        }
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
     * @returns The filename of the given path.
     */
    const std::string CatalogPublish::extractFileName(const std::string &filePath)
    {
      // Extracts the file name (e.g. CSP74683_ICPevent) from the file path.
      std::string dataFileName = Poco::Path(Poco::Path(filePath).getFileName()).getBaseName();
      // Extracts the specific file name (e.g. CSP74683) from the file path.
      return dataFileName.substr(0, dataFileName.find_first_of('_'));
    }

    /**
     * Saves the workspace (given the property) as a nexus file to the user's default directory.
     * This is then used to publish the workspace (as a file) for ease of use later.
     * @param workspace :: The workspace to save to a file.
     */
    void CatalogPublish::saveWorkspaceToNexus(Mantid::API::Workspace_sptr &workspace)
    {
      // Create the save nexus algorithm to use.
      auto saveNexus = Mantid::API::AlgorithmManager::Instance().create("SaveNexus");
      saveNexus->initialize();
      // Set the required properties & execute.
      saveNexus->setProperty("InputWorkspace", workspace->name());
      saveNexus->setProperty("FileName", Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") + workspace->name() + ".nxs");
      saveNexus->execute();
    }

    /**
     * Publish the history of a given workspace.
     * @param catalog   :: The catalog to use to publish the file.
     * @param workspace :: The workspace to obtain the history from.
     * @param datafileName :: The name of the file that is used to obtain the dataset ID.
     */
    void CatalogPublish::publishWorkspaceHistory(Mantid::API::ICatalog_sptr &catalog, Mantid::API::Workspace_sptr &workspace, std::string &datafileName)
    {
      std::stringstream ss;
      // Obtain the workspace history as a string.
      ss << generateWorkspaceHistory(workspace);
      // Use the name the use wants to save the file to the server as and append .py
      std::string fileName = Poco::Path(Poco::Path(getPropertyValue("NameInCatalog")).getFileName()).getBaseName() + ".py";
      // Publish the workspace history to the server.
      publish(ss, catalog->getUploadURL(datafileName, fileName));
    }

    /**
     * Generate the history of a given workspace.
     * @param workspace :: The workspace to obtain the history from.
     * @return The history of a given workspace.
     */
    const std::string CatalogPublish::generateWorkspaceHistory(Mantid::API::Workspace_sptr &workspace)
    {
      auto wsHistory = Mantid::API::AlgorithmManager::Instance().createUnmanaged("GeneratePythonScript");
      wsHistory->initialize();
      wsHistory->setProperty("InputWorkspace", workspace->name());
      wsHistory->setProperty("FileName", Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory") + workspace->name() + ".py");
      wsHistory->execute();
      return wsHistory->getPropertyValue("ScriptText");
    }

  }
}
