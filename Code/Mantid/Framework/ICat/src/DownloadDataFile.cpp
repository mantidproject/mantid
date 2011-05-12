#include "MantidICat/DownloadDataFile.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidICat/ErrorHandling.h" 
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/ICatalog.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"


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

    DECLARE_ALGORITHM(CDownloadDataFile)

    /// Sets documentation strings for this algorithm
    void CDownloadDataFile::initDocs()
    {
      this->setWikiSummary("Downloads the given data files from the data server ");
      this->setOptionalMessage("Downloads the given data files from the data server");
    }


    /// declaring algorithm properties
    void CDownloadDataFile::init()
    {
      declareProperty(new ArrayProperty<int64_t> ("FileIds"),"List of fileids to download from the data server");
      declareProperty(new ArrayProperty<std::string> ("FileNames"),"List of filenames to download from the data server");
      declareProperty( new ArrayProperty<std::string>("FileLocations",std::vector<std::string>(),new NullValidator<std::vector<std::string> >,
          Direction::Output),"A list of containing  locations of files downloaded from data server");
    }
    /// Execute the algorithm
    void CDownloadDataFile::exec()
    {

      ICatalog_sptr catalog_sptr;
      try
      {
        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogName());

      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
      }
      if(!catalog_sptr)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
      }
      //get file ids
      std::vector<int64_t> fileids = getProperty("FileIds");
      //get file names
      std::vector<std::string> filenames= getProperty("FileNames");

      std::vector<std::string> filelocations;
      std::vector<int64_t>::const_iterator citr1 =fileids.begin();
      std::vector<std::string>::const_iterator citr2=filenames.begin();
      m_prog =0.0;
      //loop over file ids
      for(;citr1!=fileids.end();++citr1,++citr2)
      {
        m_prog+=0.1;
        double prog=m_prog/(double(fileids.size())/10);

        std::string filelocation;
        //get the location string from catalog

        progress(prog,"getting location string...");
        catalog_sptr->getFileLocation(*citr1,filelocation);

        //if we are able to open the file from the location returned by getDatafile api
        //the user got the permission to acess archive
        std::ifstream isisfile(filelocation.c_str());
        if(isisfile)
        {
          g_log.information()<<"isis archive location for the file with id  "<<*citr1<<" is "<<filelocation<<std::endl;
          filelocations.push_back(filelocation);
        }
        else
        {
          g_log.information()<<"File with id "<<*citr1<<" can not be opened from archive,now file will be downloaded over internet from data server"<<std::endl;

          std::string url;
          progress(prog/2,"getting the url ....");
          //getting the url for the file to downlaod from respective catalog
          catalog_sptr->getDownloadURL(*citr1,url);

          progress(prog,"downloading over internet...");
          //now download the files from the data server to the machine where mantid is installed
          downloadFileOverInternet(url,*citr2);
          //get the download directory name
          std::string downloadPath( Kernel::ConfigService::Instance().getString("defaultsave.directory"));
          std::string downloadedFName=downloadPath+(*citr2);
          //replace "\" with "/"
          replaceBackwardSlash(downloadedFName);
          //set the filelocation property
          filelocations.push_back(downloadedFName);
        }

      }

      //set the filelocations  property
      setProperty("FileLocations",filelocations);

    }


    /**This method calls ICat API downloadDatafile and gets the URL string and uses the URL to down load file server
     * @param url :: url of the file to download.
     * @param fileName :: name of the file to be saved to disk
     */
    void CDownloadDataFile::downloadFileOverInternet(const std::string & url,const std::string& fileName)
    {
      //download using Poco HttpClient session and save to local disk
      doDownloadandSavetoLocalDrive(url,fileName);

    }

    /** This method checks the file extn and if it's a raw file reurns true
     * This is useful when the we download a file over internet and save to local drive,
     * to open the file in binary or ascii mode
     * @param fileName ::  file name
     * @returns true if the file is a data file
     */
    bool CDownloadDataFile::isDataFile(const std::string & fileName)
    {
      std::basic_string <char>::size_type dotIndex;
      //const std::basic_string <char>::size_type npos = -1;
      //find the position of .in row file
      dotIndex = fileName.find_last_of (".");
      std::string fextn=fileName.substr(dotIndex+1,fileName.size()-dotIndex);
      std::transform(fextn.begin(),fextn.end(),fextn.begin(),tolower);

      bool binary;
      (!fextn.compare("raw")|| !fextn.compare("nxs")) ? binary = true : binary = false;
      return binary;

    }

    /** This method downloads file over internet using Poco HTTPClientSession
     * @param URL- URL of the file to down load
     * @param fileName ::  file name
     */
    void CDownloadDataFile::doDownloadandSavetoLocalDrive(const std::string& URL,const std::string& fileName)
    {
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
        saveFiletoDisk(rs,fileName);

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

    }

    /** This method saves the input stream to a file
     * @param rs :: input stream
     * @param fileName :: name of the output file
     */
    void CDownloadDataFile::saveFiletoDisk(std::istream& rs,const std::string& fileName)
    {
      std::string filepath ( Kernel::ConfigService::Instance().getString("defaultsave.directory"));
      filepath += fileName;

      std::ios_base::openmode mode;
      //if raw/nexus file open it in binary mode else ascii
      isDataFile(fileName)? mode = std::ios_base::binary : mode = std::ios_base::out;
      std::ofstream ofs(filepath.c_str(), mode);
      if ( ofs.rdstate() & std::ios::failbit )
      {
        throw Mantid::Kernel::Exception::FileError("Error on creating File",fileName);
      }
      //copy the input stream to a file.
      StreamCopier::copyStream(rs, ofs);

    }

    /** This method is used for unit testing purpose.
     * as the Poco::Net library httpget throws an exception when the nd server n/w is slow
     * I'm testing the download from mantid server.
     * as the downlaod method I've written is private I can't access that in unit testing.
     * so adding this public method to call the private downlaod method and testing.
     * @param URL :: URL of the file to download
     * @param fileName :: name of the file
     */
    void CDownloadDataFile::testDownload(const std::string& URL,const std::string& fileName)
    {
      doDownloadandSavetoLocalDrive(URL,fileName);

    }
    /** This method replaces backward slash with forward slash for linux compatibility.
     * @param inputString :: input string
     */
    void CDownloadDataFile::replaceBackwardSlash(std::string& inputString)
    {
      std::basic_string <char>::iterator iter;
      for(iter=inputString.begin();iter!=inputString.end();++iter)
      {
        if((*iter)=='\\')
        {
          (*iter)='/';
        }
      }

    }


  }
}
