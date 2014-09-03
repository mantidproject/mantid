#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ICatalogInfoService.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidICat/CatalogDownloadDataFiles.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"

#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include <fstream>
#include <iomanip>

namespace Mantid
{
  namespace ICat
  {
    using namespace Kernel;
    using namespace API;

    DECLARE_ALGORITHM(CatalogDownloadDataFiles)


    /// declaring algorithm properties
    void CatalogDownloadDataFiles::init()
    {
      declareProperty(new ArrayProperty<int64_t> ("FileIds"),"List of fileids to download from the data server");
      declareProperty(new ArrayProperty<std::string> ("FileNames"),"List of filenames to download from the data server");
      declareProperty("DownloadPath","", "The path to save the downloaded files.");
      declareProperty("Session","","The session information of the catalog to use.");
      declareProperty(new ArrayProperty<std::string>("FileLocations",std::vector<std::string>(), 
                                                     boost::make_shared<NullValidator>(),
                                                     Direction::Output),
                      "A list of file locations to the catalog datafiles.");
    }

    /// Execute the algorithm
    void CatalogDownloadDataFiles::exec()
    {
      // Cast a catalog to a catalogInfoService to access downloading functionality.
      auto catalogInfoService = boost::dynamic_pointer_cast<API::ICatalogInfoService>(
          API::CatalogManager::Instance().getCatalog(getPropertyValue("Session")));
      // Check if the catalog created supports publishing functionality.
      if (!catalogInfoService) throw std::runtime_error("The catalog that you are using does not support external downloading.");

      // Used in order to transform the archive path to the user's operating system.
      CatalogInfo catalogInfo = ConfigService::Instance().getFacility().catalogInfo();

      std::vector<int64_t> fileIDs       = getProperty("FileIds");
      std::vector<std::string> fileNames = getProperty("FileNames");

      // Stores the paths to the related files located in the archives (if user has access).
      // Otherwise, stores the path to the downloaded file.
      std::vector<std::string> fileLocations;

      m_prog = 0.0;

      std::vector<int64_t>::const_iterator fileID       = fileIDs.begin();
      std::vector<std::string>::const_iterator fileName = fileNames.begin();

      // For every file with the given ID.
      for(; fileID != fileIDs.end(); ++fileID, ++fileName)
      {
        m_prog += 0.1;
        double prog = m_prog / (double(fileIDs.size()) /10 );

        progress(prog,"getting location string...");

        // The location of the file (on the server) stored in the archives.
        std::string fileLocation = catalogInfoService->getFileLocation(*fileID);

        g_log.debug() << "CatalogDownloadDataFiles -> File location before transform is: " << fileLocation << std::endl;
        // Transform the archive path to the path of the user's operating system.
        fileLocation = catalogInfo.transformArchivePath(fileLocation);
        g_log.debug() << "CatalogDownloadDataFiles -> File location after transform is:  " << fileLocation << std::endl;

        // Can we open the file (Hence, have access to the archives?)
        std::ifstream hasAccessToArchives(fileLocation.c_str());
        if(hasAccessToArchives)
        {
          g_log.information() << "File (" << *fileName << ") located in archives (" << fileLocation << ")." << std::endl;
          fileLocations.push_back(fileLocation);
        }
        else
        {
          g_log.information() << "Unable to open file (" << *fileName << ") from archive. Beginning to download over Internet." << std::endl;
          progress(prog/2,"getting the url ....");
          // Obtain URL for related file to download from net.
          const std::string url = catalogInfoService->getDownloadURL(*fileID);
          progress(prog,"downloading over internet...");
          const std::string fullPathDownloadedFile = doDownloadandSavetoLocalDrive(url,*fileName);
          fileLocations.push_back(fullPathDownloadedFile);
        }
      }

      // Set the fileLocations property
      setProperty("FileLocations",fileLocations);
    }

    /**
    * Checks to see if the file to be downloaded is a datafile.
    * @param fileName :: Name of data file to download.
    * @returns True if the file is a data file.
    */
    bool CatalogDownloadDataFiles::isDataFile(const std::string & fileName)
    {
      std::string extension = Poco::Path(fileName).getExtension();
      std::transform(extension.begin(),extension.end(),extension.begin(),tolower);
      return (extension.compare("raw") == 0 || extension.compare("nxs") == 0);
    }

    /**
     * Downloads datafiles from the archives, and saves to the users save default directory.
     * @param URL :: The URL of the file to download.
     * @param fileName :: The name of the file to save to disk.
     * @return The full path to the saved file.
     */
    std::string CatalogDownloadDataFiles::doDownloadandSavetoLocalDrive(const std::string& URL,const std::string& fileName)
    {
      std::string pathToDownloadedDatafile;

      clock_t start;

      try
      {
        Poco::URI uri(URL);

        std::string path(uri.getPathAndQuery());
        start=clock();

        Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> certificateHandler = new Poco::Net::AcceptCertificateHandler(true);
        // Currently do not use any means of authentication. This should be updated IDS has signed certificate.
        const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE);
        // Create a singleton for holding the default context. E.g. any future requests to publish are made to this certificate and context.
        Poco::Net::SSLManager::instance().initializeClient(NULL, certificateHandler,context);

        //Session takes ownership of socket
        Poco::Net::SecureStreamSocket* socket = new Poco::Net::SecureStreamSocket(context);
        Poco::Net::HTTPSClientSession session(*socket);
        session.setHost(uri.getHost());
        session.setPort(uri.getPort());

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
        session.sendRequest(request);

        // Close the request by requesting a response.
        Poco::Net::HTTPResponse response;
        // Store the response for use IF an error occurs (e.g. 404).
        std::istream& responseStream = session.receiveResponse(response);

        // Obtain the status returned by the server to verify if it was a success.
        Poco::Net::HTTPResponse::HTTPStatus HTTPStatus = response.getStatus();
        // The error message returned by the IDS (if one exists).
        std::string IDSError = CatalogAlgorithmHelper().getIDSError(HTTPStatus, responseStream);
        // Cancel the algorithm and display the message if it exists.
        if(!IDSError.empty())
        {
          // As an error occurred we must cancel the algorithm to prevent success message.
          this->cancel();
          // Output an appropriate error message from the JSON object returned by the IDS.
          g_log.error(IDSError);
          return "";
        }

        // Save the file to local disk if no errors occurred on the IDS.
        pathToDownloadedDatafile = saveFiletoDisk(responseStream,fileName);

        clock_t end=clock();
        float diff = float(end - start)/CLOCKS_PER_SEC;
        g_log.information()<<"Time taken to download file "<< fileName<<" is "<<std::fixed << std::setprecision(2) << diff <<" seconds" << std::endl;

      }
      catch(Poco::Net::SSLException& error)
      {
        throw std::runtime_error(error.displayText());
      }
      // A strange error occurs (what returns: {I/O error}, while message returns: { 9: The BIO reported an error }.
      // This bug has been fixed in POCO 1.4 and is noted - http://sourceforge.net/p/poco/bugs/403/
      // I have opted to catch the exception and do nothing as this allows the load/download functionality to work.
      // However, the port the user used to download the file will be left open.
      //
      // In addition, there's a crash when destructing SecureSocketImpl (internal to SecureSocketStream, which is
      // created and destroyed by HTTPSClientSession). We avoid that crash by instantiating SecureSocketStream
      // ourselves and passing it to the HTTPSClientSession, which takes ownership.
      catch(Poco::Exception& error)
      {
        throw std::runtime_error(error.displayText());
      }

      return pathToDownloadedDatafile;
    }

    /**
     * Saves the input stream to a file
     * @param rs :: The response stream from the server, which contains the file's content.
     * @param fileName :: name of the output file
     * @return Full path of where file is saved to
     */
    std::string CatalogDownloadDataFiles::saveFiletoDisk(std::istream& rs,const std::string& fileName)
    {
      std::string filepath = Poco::Path(getPropertyValue("DownloadPath"), fileName).toString();
      std::ios_base::openmode mode = isDataFile(fileName) ? std::ios_base::binary : std::ios_base::out;

      std::ofstream ofs(filepath.c_str(), mode);
      if ( ofs.rdstate() & std::ios::failbit ) throw Exception::FileError("Error on creating File",fileName);
      //copy the input stream to a file.
      Poco::StreamCopier::copyStream(rs, ofs);

      return filepath;
    }

    /**
     * This method is used for unit testing purpose.
     * as the Poco::Net library httpget throws an exception when the nd server n/w is slow
     * I'm testing the download from mantid server.
     * as the downlaod method I've written is private I can't access that in unit testing.
     * so adding this public method to call the private downlaod method and testing.
     * @param URL :: URL of the file to download
     * @param fileName :: name of the file
     * @return Full path of where file is saved to
     */
    std::string CatalogDownloadDataFiles::testDownload(const std::string& URL,const std::string& fileName)
    {
      return doDownloadandSavetoLocalDrive(URL,fileName);
    }

  }
}
