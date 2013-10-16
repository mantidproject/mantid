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
#include "MantidKernel/FileDescriptor.h"

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>
#include <fstream>
#include <iomanip>

using Poco::Net::HTTPClientSession;
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
      this->setWikiSummary("Obtains a list of file paths to the data files the user wants to download from the archives, or has downloaded and saved locally.");
    }

    /// declaring algorithm properties
    void CatalogDownloadDataFiles::init()
    {
      declareProperty(new ArrayProperty<int64_t> ("FileIds"),"List of fileids to download from the data server");
      declareProperty(new ArrayProperty<std::string> ("FileNames"),"List of filenames to download from the data server");
      declareProperty(new ArrayProperty<std::string>("Filelocations",std::vector<std::string>(),
                                                     boost::make_shared<NullValidator>(),
                                                     Direction::Output),
                      "A list of file locations to the ICAT datafiles.");
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

        // Transform the archive path to the path of the user's operating system.
        fileLocation = catalogInfo.transformArchivePath(fileLocation);

        // Can we open the file (Hence, have access to the archives?)
        std::ifstream hasAccessToArchives(fileLocation.c_str());
        if(hasAccessToArchives)
        {
          g_log.information() << "File (" << *fileName << ") located in archives." << std::endl;

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

          // Download file from the data server to the machine where Mantid is installed
          std::string fullPathDownloadedFile = doDownloadandSavetoLocalDrive(url,*fileName);

          fileLocations.push_back(fullPathDownloadedFile);
        }
      }

      // Set the fileLocations property
      setProperty("FileLocations",fileLocations);
    }

    /**
     * Checks to see if the file to be downloaded is a datafile.
     * @param fileName ::  file name
     * @returns true if the file is a data file, otherwise false.
     */
    bool CatalogDownloadDataFiles::isBinary(const std::string & fileName)
    {
      // If an invalid argument is passed (which is a test), then return false.
      try
      {
        return !FileDescriptor::isAscii(fileName);
      }
      catch(std::invalid_argument&)
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

        HTTPClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        session.sendRequest(req);

        HTTPResponse res;
        std::istream& rs = session.receiveResponse(res);
        //save file to local disk
        retVal_FullPath = saveFiletoDisk(rs,fileName);

        clock_t end=clock();
        float diff = float(end - start)/CLOCKS_PER_SEC;
        g_log.information()<<"Time taken to download file "<< fileName<<" is "<<std::fixed << std::setprecision(2) << diff <<" seconds" << std::endl;

      }
      catch(Poco::SyntaxException&)
      {
        throw std::runtime_error("Error when downloading the data file"+ fileName);
      }
      catch(Poco::Exception&)
      {
        throw std::runtime_error("Can not download the file "+fileName +". Path is invalid for the file.");
      }

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
      Poco::Path defaultSaveDir(Kernel::ConfigService::Instance().getString("defaultsave.directory"));
      Poco::Path path(defaultSaveDir, fileName);
      std::string filepath = path.toString();

      std::ios_base::openmode mode;
      //if raw/nexus file open it in binary mode else ascii
      isBinary(fileName) ? mode = std::ios_base::binary : mode = std::ios_base::out;
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
