/*WIKI*

This algorithm gets the location strings for the selected files from the data archive;
if the data archive is not accessible, it downloads the files from the data server.

*WIKI*/

#include "MantidICat/CatalogDownloadDataFiles.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/ICatalog.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <fstream>
#include <iomanip>

using Poco::Net::HTTPSClientSession;
using Poco::Net::Context;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::Exception;

namespace Mantid
{
  namespace ICat
  {
    using namespace Kernel;
    using namespace API;

    DECLARE_ALGORITHM(CatalogDownloadDataFiles)

    /// Sets documentation strings for this algorithm
    void CatalogDownloadDataFiles::initDocs()
    {
      this->setWikiSummary("Downloads the given data files from the data server ");
      this->setOptionalMessage("Downloads the given data files from the data server");
    }


    /// declaring algorithm properties
    void CatalogDownloadDataFiles::init()
    {
      declareProperty(new ArrayProperty<int64_t> ("FileIds"),"List of fileids to download from the data server");
      declareProperty(new ArrayProperty<std::string> ("FileNames"),"List of filenames to download from the data server");
      declareProperty("DownloadPath","", "The path to save the files to download to.");
      declareProperty(new ArrayProperty<std::string>("FileLocations",std::vector<std::string>(), 
                                                     boost::make_shared<NullValidator>(),
                                                     Direction::Output),
                      "A list of file locations to the catalog datafiles.");
    }

    /// Raise an error concerning catalog searching
    void CatalogDownloadDataFiles::throwCatalogError() const
    {
      const std::string facilityName = ConfigService::Instance().getFacility().name();
      std::stringstream ss;
      ss << "Your current Facility, " << facilityName << ", does not have ICAT catalog information. "
          << std::endl;
      ss << "The facilities.xml file may need updating. Contact the Mantid Team for help." << std::endl;
      throw std::runtime_error(ss.str());
    }

    /// Execute the algorithm
    void CatalogDownloadDataFiles::exec()
    {
      ICatalog_sptr catalog;

      try
      {
        catalog = CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogInfo().catalogName());

      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throwCatalogError();
      }

      if(!catalog)
      {
        throwCatalogError();
      }

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

        // The location of the file on the ICAT server (E.g. in the archives).
        std::string fileLocation;
        catalog->getFileLocation(*fileID,fileLocation);

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
          std::string url;
          catalog->getDownloadURL(*fileID,url);

          progress(prog,"downloading over internet...");

          std::string fullPathDownloadedFile = doDownloadandSavetoLocalDrive(url,*fileName);

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

      if (extension.compare("raw") == 0 || extension.compare("nxs") == 0)
      {
        return true;
      }
      else
      {
        return false;
      }
    }

    /**
     * Downloads file over Internet using Poco HTTPClientSession
     * @param URL- URL of the file to down load
     * @param fileName ::  file name
     * @return Full path of where file is saved to
     */
    std::string CatalogDownloadDataFiles::doDownloadandSavetoLocalDrive(const std::string& URL,const std::string& fileName)
    {
      std::string retVal_FullPath;

      clock_t start;
      //use HTTP  Get method to download the data file from the server to local disk
      try
      {
        URI uri(URL);

        std::string path(uri.getPathAndQuery());
        if (path.empty())
        {
          throw std::runtime_error("URL string is empty,ICat interface can not download the file"+fileName);
        }
        start=clock();

        // Currently do not use any means of authentication. This should be updated IDS has signed certificate.
        const Context::Ptr context = new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE);
        HTTPSClientSession session(uri.getHost(), uri.getPort(), context);

        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        session.sendRequest(request);

        HTTPResponse res;
        std::istream& rs = session.receiveResponse(res);

        //save file to local disk
        retVal_FullPath = saveFiletoDisk(rs,fileName);

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
      catch(Poco::Exception&) {}

      return retVal_FullPath;
    }

    /**
     * Saves the input stream to a file
     * @param rs :: input stream
     * @param fileName :: name of the output file
     * @return Full path of where file is saved to
     */
    std::string CatalogDownloadDataFiles::saveFiletoDisk(std::istream& rs,const std::string& fileName)
    {
      std::string downloadPath = getProperty("DownloadPath");
      Poco::Path defaultSaveDir(downloadPath);
      Poco::Path path(downloadPath, fileName);
      std::string filepath = path.toString();

      std::ios_base::openmode mode = isDataFile(fileName) ? std::ios_base::binary : std::ios_base::out;

      std::ofstream ofs(filepath.c_str(), mode);
      if ( ofs.rdstate() & std::ios::failbit )
      {
        throw Mantid::Kernel::Exception::FileError("Error on creating File",fileName);
      }

      //copy the input stream to a file.
      StreamCopier::copyStream(rs, ofs);

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
