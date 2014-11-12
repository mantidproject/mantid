#include "MantidDataHandling/DownloadInstrument.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NetworkProxy.h"
#include "MantidKernel/ProxyInfo.h"

// from poco
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/TemporaryFile.h>
#include <Poco/URI.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Exception.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/NetException.h>
#include "Poco/Net/FilePartSource.h"
// Visual Studio complains with the inclusion of Poco/FileStream
// disabling this warning.
#if defined(_WIN32) || defined(_WIN64)
#pragma warning( push )
#pragma warning( disable : 4250 )
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <Winhttp.h>
#pragma warning( pop )
#else
#include <Poco/FileStream.h>
#include <Poco/NullStream.h>
#include <stdlib.h>
#endif
#include <Poco/StreamCopier.h>
#include <Poco/Net/NetException.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeFormat.h>

// from boost
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/regex.hpp>

using namespace Poco::Net;
using boost::property_tree::ptree;

namespace Mantid
{
  namespace DataHandling
  {

    using Mantid::Kernel::Direction;
    using Mantid::API::WorkspaceProperty;

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(DownloadInstrument)



    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    DownloadInstrument::DownloadInstrument():remote_url("api.github.com")
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    DownloadInstrument::~DownloadInstrument()
    {
    }


    //----------------------------------------------------------------------------------------------

    /// Algorithms name for identification. @see Algorithm::name
    const std::string DownloadInstrument::name() const { return "DownloadInstrument"; }

    /// Algorithm's version for identification. @see Algorithm::version
    int DownloadInstrument::version() const { return 1;};

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string DownloadInstrument::category() const { return "DataHandling\\Instrument";}

    /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
    const std::string DownloadInstrument::summary() const { return "Downloads one or more instruments from the Mantid instrument repository";};

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void DownloadInstrument::init()
    {

    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void DownloadInstrument::exec()
    {

      Remote2LocalFileMap& fileMap = processRepository();

      for (auto itMap = fileMap.begin(); itMap != fileMap.end(); ++itMap)
      {
        //download a file
        doDownloadFile(itMap->first, itMap->second);
      }
    }


    Remote2LocalFileMap DownloadInstrument::processRepository()
    {
      Remote2LocalFileMap fileMap;


      //get the instrument directories
      auto instrumentDirs = Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();
      Poco::Path installPath(instrumentDirs[instrumentDirs.size()-1]);
      installPath.makeDirectory();
      Poco::Path localPath(instrumentDirs[0]);
      localPath.makeDirectory();

      //update local repo files
      Poco::Path installRepoFile(localPath);
      installRepoFile.append("install.json");
      updateJsonFile(installPath.toString(),installRepoFile.toString());      
      Poco::Path localRepoFile(localPath);
      localRepoFile.append("local.json");
      updateJsonFile(localPath.toString(),localRepoFile.toString());

      //get the file list from github
      Poco::Path gitHubJson(localPath);
      gitHubJson.append("github.json").toString();
      doDownloadFile("https://api.github.com/repos/mantidproject/mantid/contents/Code/Mantid/instrument", gitHubJson.toString());

      //Parse the server JSON response
      ptree ptGithub; 
      //and the local JSON files
      ptree ptLocal;      
      ptree ptInstall;
      try
      {
        read_json(gitHubJson.toString(), ptGithub);
        read_json(installRepoFile.toString(), ptInstall);
        read_json(localRepoFile.toString(), ptLocal);

        BOOST_FOREACH(ptree::value_type & repoFile, ptGithub)
        {
          std::string name = repoFile.second.get("name","");
          std::string path = repoFile.second.get("path","");
          std::string sha = repoFile.second.get("sha","");
          std::string htmlUrl = repoFile.second.get("html_url","");
          htmlUrl += "?raw=1";

          Poco::Path filePath(localPath);
          filePath.append(name);
          if (filePath.getExtension() == "xml")
          {
            //decide if we want to download this file

            std::string keyBase = MangleFileName(name);
            //read sha from local directories
            std::string localSha = ptLocal.get(keyBase + ".sha","");
            std::string installSha = ptInstall.get(keyBase + ".sha","");

            // Different sha1 on github cf local and global
            // this will also catch when file is only present on github (as local sha will be "")
            if ((sha != installSha) && (sha != localSha))
            {
              fileMap.insert(std::make_pair(htmlUrl, filePath.toString())); // ACTION - DOWNLOAD to localPath
            }
            else if ((localSha != "") && (sha == installSha) && (sha != localSha)) // matches install, but different local
            {
              fileMap.insert(std::make_pair(htmlUrl, filePath.toString())); // ACTION - DOWNLOAD to localPath and overwrite
            }
          }
        }
      } 
      catch (boost::property_tree::json_parser_error & ex)
      {
        throw std::runtime_error(ex.what());
      }


      return fileMap;

    }

    /** creates or updates the json file of a directories contents
    * @param directoryPath The path to catalog
    * @param filePath The path of the file containing the datalog
    **/
    void DownloadInstrument::updateJsonFile(const std::string& directoryPath, const std::string& filePath)
    {
      ptree pt;
      //check if the file exists
      Poco::File catalogFile(filePath);
      if (catalogFile.exists() && catalogFile.isFile())
      {
        try
        {
          read_json(filePath, pt);
        } 
        catch (boost::property_tree::json_parser_error & ex)
        {
          throw std::runtime_error(ex.what());
        }
      }

      using Poco::DirectoryIterator;
      DirectoryIterator end;
      try
      {
        for (DirectoryIterator it(directoryPath); it != end; ++it)
        {
          Poco::Path entryPath = it->path();
          std::string entryExt = entryPath.getExtension();
          if (entryExt == "xml")
          {
            //get current values
            std::string entryName = entryPath.getBaseName();
            Poco::LocalDateTime dateTime(it->getLastModified());
            size_t entrySize = it->getSize();

            //read previous values
            std::string keyBase = MangleFileName(it->path());
            size_t previousSize = pt.get(keyBase + ".size",0);
            std::string pdtString = pt.get(keyBase + ".lastModified","1900-01-01 00:00:00");
            int tzd(0);
            Poco::DateTime previousDateTime;
            Poco::DateTimeParser::tryParse(Poco::DateTimeFormat::SORTABLE_FORMAT, pdtString, previousDateTime, tzd);
            previousDateTime += Poco::Timespan(1,0); //add a second as milliseconds are truncated off in the file
            std::string prevSha = pt.get(keyBase + ".sha","");

            //update and generate sha if anything has changed
            if ((entrySize != previousSize) || (dateTime > previousDateTime) || (prevSha == ""))
            {
              pt.put(keyBase + ".size",entrySize);
              pt.put(keyBase + ".path",entryPath.toString());
              pt.put(keyBase + ".lastModified",Poco::DateTimeFormatter::format(dateTime, Poco::DateTimeFormat::SORTABLE_FORMAT));
              pt.put(keyBase + ".sha",Kernel::ChecksumHelper::gitSha1FromFile(entryPath.toString()));
            }
          }
        }
      } catch (Poco::Exception & ex)
      {
        g_log.error() << "DownloadInstrument: failed to parse the directory: " << directoryPath << " : "
          << ex.className() << " : " << ex.displayText() << std::endl;
        // silently ignore this exception.
        // throw ScriptRepoException(ex.displayText());
      } catch (std::exception & ex)
      {
        std::stringstream ss;
        ss << "unknown exception while checking local file system. " << ex.what() << ". Input = "
          << directoryPath;
        g_log.error() << "DownloadInstrument: " << ss.str() << std::endl;
        throw std::runtime_error(ss.str());
      }

      //now write the updated file
      write_json(filePath, pt);

    }

    /** Converts a filename into a valid key for a boost property tree.
    * @param filename The filename and extension with or without path
    * @returns a mangled filename that is valid for use as a property tree key
    **/
    const std::string DownloadInstrument::MangleFileName(const std::string& filename) const
    {
      Poco::Path entryPath(filename);
      std::string entryExt = entryPath.getExtension();
      std::string entryName = entryPath.getBaseName();              
      std::stringstream ss;
      ss << entryName << "_" << entryExt;
      return ss.str();
    }
//https://github.com/mantidproject/mantid/blob/master/Code/Mantid/instrument/ALF_Definition.xml
//https://raw.githubusercontent.com/mantidproject/mantid/master/Code/Mantid/instrument/ALF_Definition.xml
    /** Download a url and fetch it inside the local path given.

    This method was initially modelled on doDownloadFile from the ScriptRepositoryImpl 
    and then converted to handle SSL connections

    @param urlFile: Define a valid URL for the file to be downloaded. Eventually, it may give
    any valid https path. For example:

    url_file = "https://www.google.com"

    url_file = "https://mantidweb/repository/README.md"

    The result is to connect to the http server, and request the path given.

    The answer, will be inserted at the local_file_path.

    @param localFilePath [optional] : Provide the destination of the file downloaded at the url_file.
    the connection and the download was done correctly.

    @exception std::runtime_error : For any unexpected behaviour.
    */
    void DownloadInstrument::doDownloadFile(const std::string & urlFile,
      const std::string & localFilePath)
    {
      g_log.debug() << "DoDownloadFile : " << urlFile << " to file: " << localFilePath << std::endl;

      Poco::URI uri(urlFile);
      std::stringstream answer;
      try {
        // initialize ssl
        Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> pHandler = new AcceptCertificateHandler(true); // was false
        Context::Ptr pContext = new Context(Context::CLIENT_USE, "", Context::VERIFY_NONE);
        SSLManager::instance().initializeClient(0, pHandler, pContext);

        // Session takes ownership of socket
        HTTPSClientSession session(uri.getHost(), uri.getPort(), pContext);
        Mantid::Kernel::NetworkProxy proxyHelper;
        Mantid::Kernel::ProxyInfo proxyInfo = proxyHelper.getHttpProxy(remote_url);
        if (!proxyInfo.emptyProxy())
        {
          session.setProxyHost(proxyInfo.host());
          session.setProxyPort(proxyInfo.port());
        }

        // create a request
        HTTPRequest req(HTTPRequest::HTTP_GET, uri.getPathAndQuery(),
          HTTPMessage::HTTP_1_1);
        req.set("User-Agent","MANTID-Instrument-Repo");
        session.sendRequest(req);

        Poco::Net::HTTPResponse res;
        std::istream & rs = session.receiveResponse(res);
        g_log.debug() << "Answer from web: " << res.getStatus() << " "
          << res.getReason() << std::endl;
        if (res.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
        {
          if (localFilePath.empty())
          {
            // ignore the answer, throw it away
            Poco::NullOutputStream null;
            Poco::StreamCopier::copyStream(rs, null);
            return;
          }
          else
          {
            // copy the file
            Poco::FileStream _out(localFilePath);
            Poco::StreamCopier::copyStream(rs, _out);
            _out.close();
          }
        }
        else if (res.getStatus() == Poco::Net::HTTPResponse::HTTP_FOUND)
        {
          //extract the new location
          std::string newLocation = res.get("location","");
          if (newLocation != "")
          {
            doDownloadFile(newLocation,localFilePath);
          }
        }
        else
        {
          std::stringstream info;
          std::stringstream ss;
          Poco::StreamCopier::copyStream(rs, ss);
          if (res.getStatus() == Poco::Net::HTTPResponse::HTTP_NOT_FOUND)
            info << "Failed to download " << urlFile
            << " because it failed to find this file at the link " << "<a href=\"" << urlFile
            << "\">.\n" << "Hint. Check that link is correct</a>";
          else
          {
            // show the error
            info << res.getReason();
            info << ss.str();
          }
          throw std::runtime_error(info.str() + ss.str());
        }
      } catch (Poco::Net::HostNotFoundException & ex)
      {
        // this exception occurs when the pc is not connected to the internet
        std::stringstream info;
        info << "Failed to download " << urlFile << " because there is no connection to the host "
          << ex.message() << ".\nHint: Check your connection following this link: <a href=\""
          << urlFile << "\">" << urlFile << "</a>";
        throw std::runtime_error(info.str() + ex.displayText());

      } catch (Poco::Exception & ex)
      {
        throw Poco::Exception("Connection and request failed", ex);
      }
    }



  } // namespace DataHandling
} // namespace Mantid