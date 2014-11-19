#include "MantidDataHandling/DownloadInstrument.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NetworkProxy.h"

// Poco
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/SSLManager.h>
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
#include <Poco/URI.h>

// jsoncpp
#include <jsoncpp/json/json.h>

// std
#include <fstream>

namespace Mantid
{
  namespace DataHandling
  {
    using namespace Kernel;
    using namespace Poco::Net;

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(DownloadInstrument)

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    DownloadInstrument::DownloadInstrument() : m_proxyInfo(), m_isProxySet(false)
    {
    }

    //----------------------------------------------------------------------------------------------

    /// Algorithms name for identification. @see Algorithm::name
    const std::string DownloadInstrument::name() const { return "DownloadInstrument"; }

    /// Algorithm's version for identification. @see Algorithm::version
    int DownloadInstrument::version() const { return 1;}

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string DownloadInstrument::category() const { return "DataHandling\\Instrument";}

    /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
    const std::string DownloadInstrument::summary() const
    { 
      return "Checks the Mantid instrument repository against the local "
             "instrument files, and downloads updates as appropriate.";
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void DownloadInstrument::init()
    {
      using Kernel::Direction;

      declareProperty("FileDownloadCount", 0,
                      "The number of files downloaded by this algorithm", Direction::Output);
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void DownloadInstrument::exec()
    {
      StringToStringMap fileMap;
      setProperty("FileDownloadCount",0);
      try
      {
        fileMap = processRepository();
      }
      catch (Mantid::Kernel::Exception::InternetError & ex)
      {
        std::string errorText(ex.what());
        if (errorText.find("rate limit") != std::string::npos)
        {
          g_log.notice()<< "Instrument Definition Update: " << errorText << std::endl;
        }
        else
        {
          //log the failure at Notice Level
          g_log.notice()<< "Internet Connection Failed - cannot update instrument definitions." << std::endl;
          //log this error at information level
          g_log.information() << errorText <<std::endl;
        }
        return;
      }

      if (fileMap.size() == 0)
      {
        g_log.notice("All instrument definitions up to date");
      }
      else
      {
        std::string s = (fileMap.size()>1)?"s":"";
        g_log.notice()<<"Downloading " << fileMap.size() << " file" << s << " from the instrument repository" << std::endl;
      }

      for (auto itMap = fileMap.begin(); itMap != fileMap.end(); ++itMap)
      {
        //download a file
        doDownloadFile(itMap->first, itMap->second);
      }
      
      setProperty("FileDownloadCount",static_cast<int>(fileMap.size()));
    }
    
    DownloadInstrument::StringToStringMap DownloadInstrument::processRepository()
    {
      //get the instrument directories
      auto instrumentDirs = Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();
      Poco::Path installPath(instrumentDirs[instrumentDirs.size()-1]);
      installPath.makeDirectory();
      Poco::Path localPath(instrumentDirs[0]);
      localPath.makeDirectory();

      //get the date of the local github.json file if it exists
      Poco::Path gitHubJson(localPath, "github.json");
      Poco::File gitHubJsonFile(gitHubJson);
      Poco::DateTime gitHubJsonDate(1900,1,1);
      if (gitHubJsonFile.exists() && gitHubJsonFile.isFile())
      {
        gitHubJsonDate = gitHubJsonFile.getLastModified();
      }

      //get the file list from github
      StringToStringMap headers;
      headers.insert(std::make_pair("if-modified-since",
                                    Poco::DateTimeFormatter::format(gitHubJsonDate, Poco::DateTimeFormat::HTTP_FORMAT)));
      std::string gitHubInstrumentRepoUrl = ConfigService::Instance().getString("UpdateInstrumentDefinitions.URL");
      if (gitHubInstrumentRepoUrl == "")
      {
        throw std::runtime_error("Property UpdateInstrumentDefinitions.URL is not defined, "
                                 "this should point to the location of the instrument "
                                 "directory in the github API "
                                 "e.g. https://api.github.com/repos/mantidproject/mantid/contents/Code/Mantid/instrument.");
      }
      StringToStringMap fileMap;
      if (doDownloadFile(gitHubInstrumentRepoUrl, gitHubJson.toString(),headers) == HTTPResponse::HTTP_NOT_MODIFIED)
      {
        //No changes since last time
        return fileMap;
      }

      //update local repo files
      Poco::Path installRepoFile(localPath, "install.json");
      StringToStringMap installShas = updateJsonFile(installPath.toString(), installRepoFile.toString());
      Poco::Path localRepoFile(localPath, "local.json");
      StringToStringMap localShas = updateJsonFile(localPath.toString(), localRepoFile.toString());

      // Parse the server JSON response
      Json::Reader reader;
      Json::Value serverContents;
      Poco::FileStream fileStream(gitHubJson.toString(), std::ios::in);
      if(!reader.parse(fileStream, serverContents))
      {
        throw std::runtime_error("Unable to parse server JSON file \"" + gitHubJson.toString() + "\"");
      }
      fileStream.close();

      for(Json::ArrayIndex i = 0; i < serverContents.size(); ++i)
      {
        const auto & serverElement = serverContents[i];
        std::string name = serverElement.get("name", "").asString();
        Poco::Path filePath(localPath, name);
        if(filePath.getExtension() != "xml") continue;
        std::string sha = serverElement.get("sha","").asString();
        std::string htmlUrl = getDownloadableRepoUrl(serverElement.get("html_url","").asString());

        // Find shas
        std::string localSha = getValueOrDefault(localShas, name, "");
        std::string installSha = getValueOrDefault(installShas, name, "");
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
      return fileMap;
    }

    /**
     *
     * @param mapping A map of string keys to string values
     * @param key A string representing a key
     * @param defaultValue A default to return if the key is not present
     * @return The value of the key or the default if the key does not exist
     */
    std::string
    DownloadInstrument::getValueOrDefault(const DownloadInstrument::StringToStringMap &mapping,
                                          const std::string &key, const std::string &defaultValue) const
    {
      auto element = mapping.find(key);
      return (element != mapping.end()) ? element->second : defaultValue;
    }

    /** Creates or updates the json file of a directories contents
    * @param directoryPath The path to catalog
    * @param filePath The path of the file containing the datalog
    * @return A map of file names to sha1 values
    **/
    DownloadInstrument::StringToStringMap
    DownloadInstrument::updateJsonFile(const std::string& directoryPath, const std::string& filePath)
    {
      Json::Value root;
      Json::Reader reader;
      //check if the file exists
      Poco::File catalogFile(filePath);
      if (catalogFile.exists() && catalogFile.isFile())
      {
        std::ifstream catalogStream(filePath, std::ios::in);
        if (!reader.parse(catalogStream, root))
        {
          throw std::runtime_error("Unable to parse JSON file \"" + filePath + "\"");
        }
      }
      
      // -- File layout -- 
      // 
      // [
      //   {
      //     "name": "ALF_Definition.xml",
      //     "lastModified": "1900-01-01 00:00:00",
      //     "path": "/path/to/file.xml"
      //     "sha": "fff6fe3a23bf1c8ea0692b4a883af99bee26fd3b",
      //     "size": 625
      //   },
      // ...
      // ]

      StringToStringMap filesToSha;
      try
      {
        using Poco::DirectoryIterator;
        DirectoryIterator end;
        for (DirectoryIterator it(directoryPath); it != end; ++it)
        {
          const auto & entryPath = Poco::Path(it->path());
          if (entryPath.getExtension() != "xml") continue;

          Json::Value & element = root.append(Json::Value());
          element["name"] = entryPath.getFileName();
          element["lastModified"] = Poco::DateTimeFormatter::format(it->getLastModified(), 
                                                                    Poco::DateTimeFormat::SORTABLE_FORMAT);
          element["path"] = entryPath.toString();
          std::string sha1 = ChecksumHelper::gitSha1FromFile(entryPath.toString());
          element["sha"] = sha1;
          element["size"] = static_cast<Json::UInt64>(it->getSize());
          // Track sha1
          filesToSha.insert(std::make_pair(entryPath.getFileName(), sha1));

//          //get current values
//          Poco::LocalDateTime dateTime(it->getLastModified());
//          size_t entrySize = it->getSize();
          
//          //  read previous values
//          std::string keyBase = mangleFileName(it->path());
//          Json::UInt64 previousSize = pt.get(keyBase + ".size", 0).asUInt64();
//          std::string pdtString = pt.get(keyBase + ".lastModified","1900-01-01 00:00:00").asString();
//          int tzd(0);
//          Poco::DateTime previousDateTime;
//          Poco::DateTimeParser::tryParse(Poco::DateTimeFormat::SORTABLE_FORMAT, pdtString, previousDateTime, tzd);
//          previousDateTime += Poco::Timespan(1,0); //add a second as milliseconds are truncated off in the file
//          std::string prevSha = pt.get(keyBase + ".sha","");
          
//          //update and generate sha if anything has changed
//          if ((entrySize != previousSize) || (dateTime > previousDateTime) || (prevSha == ""))
//          {
                
//            pt.put(keyBase + ".size", static_cast<Json::UInt64>(entrySize));
//            pt.put(keyBase + ".path", entryPath.toString());
//            pt.put(keyBase + ".lastModified", Poco::DateTimeFormatter::format(dateTime, Poco::DateTimeFormat::SORTABLE_FORMAT));
//            pt.put(keyBase + ".sha", ChecksumHelper::gitSha1FromFile(entryPath.toString()));
//          }
        }
      }
      catch (Poco::Exception & ex)
      {
        g_log.error() << "DownloadInstrument: failed to parse the directory: " << directoryPath << " : "
          << ex.className() << " : " << ex.displayText() << std::endl;
        // silently ignore this exception.
      } catch (std::exception & ex)
      {
        std::stringstream ss;
        ss << "unknown exception while checking local file system. " << ex.what() << ". Input = "
           << directoryPath;
        throw std::runtime_error(ss.str());
      }

      // Persist file contents
      Poco::FileStream fileOut(filePath, std::ios::out);
      fileOut << root;

      return filesToSha;
    }

    /** Converts a filename into a valid key for a boost property tree.
    * @param filename The filename and extension with or without path
    * @returns a mangled filename that is valid for use as a property tree key
    **/
    const std::string DownloadInstrument::mangleFileName(const std::string& filename) const
    {
      Poco::Path entryPath(filename);
      std::string entryExt = entryPath.getExtension();
      std::string entryName = entryPath.getBaseName();
      std::stringstream ss;
      ss << entryName << "_" << entryExt;
      return ss.str();
    }

    /** Converts a github file page to a downloadable url for the file.
    * @param filename a github file page url
    * @returns a downloadable url for the file
    **/
    const std::string DownloadInstrument::getDownloadableRepoUrl(const std::string& filename) const
    {
      return filename + "?raw=1";
    }

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

    @param headers [optional] : A key value pair map of any additional headers to include in the request.

    @exception Mantid::Kernel::Exception::InternetError : For any unexpected behaviour.
    */
    int DownloadInstrument::doDownloadFile(const std::string & urlFile,
      const std::string & localFilePath,
      const StringToStringMap & headers)
    {
      int retStatus = 0;
      g_log.debug() << "DoDownloadFile : " << urlFile << " to file: " << localFilePath << std::endl;

      Poco::URI uri(urlFile);
      try {
        // initialize ssl
        Poco::SharedPtr<InvalidCertificateHandler> certificateHandler = \
          new AcceptCertificateHandler(true);
        // Currently do not use any means of authentication. This should be updated IDS has signed certificate.
        const Context::Ptr context = \
          new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE);
        // Create a singleton for holding the default context.
        // e.g. any future requests to publish are made to this certificate and context.
        SSLManager::instance().initializeClient(NULL, certificateHandler, context);
        // Create the session
        HTTPSClientSession session(uri.getHost(), static_cast<Poco::UInt16>(uri.getPort()));

        //set the proxy
        if (!m_isProxySet)
        {
          Mantid::Kernel::NetworkProxy proxyHelper;
          m_proxyInfo = proxyHelper.getHttpProxy(uri.getHost());
          m_isProxySet = true;
        }
        if (!m_proxyInfo.emptyProxy())
        {
          session.setProxyHost(m_proxyInfo.host());
          session.setProxyPort(static_cast<Poco::UInt16>(m_proxyInfo.port()));
        }

        // create a request
        HTTPRequest req(HTTPRequest::HTTP_GET, uri.getPathAndQuery(),
                        HTTPMessage::HTTP_1_1);
        req.set("User-Agent","MANTID");
        for (auto itHeaders = headers.begin(); itHeaders != headers.end(); ++itHeaders)
        {
          req.set(itHeaders->first,itHeaders->second);
        }
        session.sendRequest(req);

        HTTPResponse res;
        std::istream & rs = session.receiveResponse(res);
        retStatus = res.getStatus();
        g_log.debug() << "Answer from web: " << res.getStatus() << " "
                      << res.getReason() << std::endl;
        
        //get github api rate limit information if available;
        int rateLimitRemaining;
        DateAndTime rateLimitReset;
        try 
        {
          rateLimitRemaining = boost::lexical_cast<int>( res.get("X-RateLimit-Remaining","-1") );
          rateLimitReset.set_from_time_t(boost::lexical_cast<int>( res.get("X-RateLimit-Reset","0")));
        }
        catch( boost::bad_lexical_cast const& ) 
        {
          rateLimitRemaining = -1;
        }

        if (res.getStatus() == HTTPResponse::HTTP_OK)
        {
          if (localFilePath.empty())
          {
            // ignore the answer, throw it away
            Poco::NullOutputStream null;
            Poco::StreamCopier::copyStream(rs, null);
            return retStatus;
          }
          else
          {
            // copy the file
            Poco::FileStream _out(localFilePath);
            Poco::StreamCopier::copyStream(rs, _out);
            _out.close();
          }
        }
        else if (res.getStatus() == HTTPResponse::HTTP_FOUND)
        {
          //extract the new location
          std::string newLocation = res.get("location","");
          if (newLocation != "")
          {
            retStatus = doDownloadFile(newLocation,localFilePath);
          }
        }
        else if (res.getStatus() == HTTPResponse::HTTP_NOT_MODIFIED)
        {
          //do nothing - just return the status
        }        
        else if ((res.getStatus() == HTTPResponse::HTTP_FORBIDDEN) && (rateLimitRemaining == 0))
        {
          throw Exception::InternetError("The Github API rate limit has been reached, try again after " + 
                                         rateLimitReset.toSimpleString(),res.getStatus());
        }
        else
        {
          std::stringstream info;
          std::stringstream ss;
          Poco::StreamCopier::copyStream(rs, ss);
          if (res.getStatus() == HTTPResponse::HTTP_NOT_FOUND)
            info << "Failed to download " << urlFile
            << " because it failed to find this file at the link " << "<a href=\"" << urlFile
            << "\">.\n" << "Hint. Check that link is correct</a>";
          else
          {
            // show the error
            info << res.getReason();
            info << ss.str();
          }
          throw Exception::InternetError(info.str() + ss.str(),res.getStatus());
        }
      } catch (HostNotFoundException & ex)
      {
        // this exception occurs when the pc is not connected to the internet
        std::stringstream info;
        info << "Failed to download " << urlFile << " because there is no connection to the host "
          << ex.message() << ".\nHint: Check your connection following this link: <a href=\""
          << urlFile << "\">" << urlFile << "</a> ";
        throw Exception::InternetError(info.str() + ex.displayText());

      } catch (Poco::Exception & ex)
      {
        throw Exception::InternetError("Connection and request failed " + ex.displayText());
      }
      return retStatus;
    }



  } // namespace DataHandling
} // namespace Mantid
