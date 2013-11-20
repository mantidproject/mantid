#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/Progress.h"
#include "MantidICat/ICat4/GSoapGenerated/ICat4ICATPortBindingProxy.h"
#include "MantidICat/ICat4/ICat4Catalog.h"
#include "MantidICat/Session.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Strings.h"

namespace Mantid
{
  namespace ICat
  {
    using namespace Kernel;
    using namespace ICat4;

    DECLARE_CATALOG(ICat4Catalog)

    /// Destructor
    ICat4Catalog::~ICat4Catalog() { }

    /**
     * Uses ICat API login to connect to catalog.
     * @param username :: login name(E.g. federal id) of the user
     * @param password :: password of the user
     * @param url      :: endpoint url of the catalog
     */
    void ICat4Catalog::login(const std::string& username, const std::string& password, const std::string& url)
    {
      UNUSED_ARG(url)
      ICat4::ICATPortBindingProxy icat;

      // Define ssl authentication scheme
      setSSLContext(icat);

      // Used to authenticate the user.
      ns1__login login;
      ns1__loginResponse loginResponse;

      // Used to add entries to the login class.
      _ns1__login_credentials_entry entry;

      // Name of the authentication plugin in use.
      std::string plugin("uows");
      login.plugin = &plugin;

      // Making string as cannot convert from const.
      std::string userName(username);
      std::string passWord(password);

      std::string usernameKey("username");
      std::string passwordKey("password");

      // Instantiate an instance of an entry to prevent null pointer.
      // This then allows us to push entries as required below.
      std::vector<_ns1__login_credentials_entry> entries;
      login.credentials.entry = &entries;

      // Setting the username and pass credentials to the login class.
      entry.key   = &usernameKey;
      entry.value = &userName;
      entries.push_back(entry);

      entry.key   = &passwordKey;
      entry.value = &passWord;
      entries.push_back(entry);

      // Is the login successful? (0 if yes, 12 if no).
      int result = icat.login(&login, &loginResponse);

      if (result == 0)
      {
        std::string session_id = *(loginResponse.return_);
        ICat::Session::Instance().setSessionId(session_id);
        ICat::Session::Instance().setUserName(userName);
      }
      else
      {
        throw std::runtime_error("Username or password supplied is invalid.");
      }
    }

    /**
     * Disconnects the client application from ICat4 based catalog services.
     */
    void ICat4Catalog::logout()
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__logout request;
      ns1__logoutResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      int result = icat.logout(&request,&response);

      // If session is set and valid.
      if(result == 0)
      {
        Session::Instance().setSessionId("");
      }
      else
      {
        throw std::runtime_error("You are not currently logged into the cataloging system.");
      }
    }

    /**
     * Creates a search query string based on inputs provided by the user.
     * @param inputs :: reference to a class contains search inputs.
     * @param offset :: skip this many rows and start returning rows from this point.
     * @param limit  :: limit the number of rows returned by the query.
     * @return a query string constructed from user input.
     */
    std::string ICat4Catalog::getSearchQuery(const CatalogSearchParam& inputs, const int &offset, const int &limit)
    {
      // Contain the related where and join clauses for the search query based on user-input.
      std::vector<std::string> whereClause, joinClause;

      // Format the timestamps in order to compare them.
      std::string startDate = formatDateTime(inputs.getStartDate(), "%Y-%m-%d %H:%M:%S");
      std::string endDate   = formatDateTime(inputs.getEndDate() + ((23*60*60) + (59*60) + 59), "%Y-%m-%d %H:%M:%S");

      // Investigation startDate if endDate is not selected
      if (inputs.getStartDate() != 0 && inputs.getEndDate() == 0)
      {
        whereClause.push_back("inves.startDate >= '" + startDate + "'");
      }

      // Investigation endDate if startdate is not selected
      if (inputs.getEndDate() != 0 && inputs.getStartDate() == 0)
      {
        whereClause.push_back("inves.endDate <= '" + endDate + "'");
      }

      // Investigation Start and end date if both selected
      if(inputs.getStartDate() != 0 && inputs.getEndDate() != 0)
      {
        whereClause.push_back("inves.startDate BETWEEN '" + startDate + "' AND '" + endDate + "'");
      }

      // Investigation name (title)
      if(!inputs.getInvestigationName().empty())
      {
        whereClause.push_back("inves.title LIKE '%" + inputs.getInvestigationName() + "%'");
      }

      // Investigation type
      if(!inputs.getInvestigationType().empty())
      {
        joinClause.push_back("JOIN inves.type itype");
        whereClause.push_back("itype.name = '" + inputs.getInvestigationType() + "'");
      }

      // Instrument name
      if(!inputs.getInstrument().empty())
      {
        joinClause.push_back("JOIN inves.investigationInstruments invInst");
        joinClause.push_back("JOIN invInst.instrument inst");
        whereClause.push_back("inst.name = '" + inputs.getInstrument() + "'");
      }

      // Keywords
      if(!inputs.getKeywords().empty())
      {
        joinClause.push_back("JOIN inves.keywords keywords");
        whereClause.push_back("keywords.name IN ('" + inputs.getKeywords() + "')");
      }

      // Sample name
      if(!inputs.getSampleName().empty())
      {
        joinClause.push_back("JOIN inves.samples sample");
        whereClause.push_back("sample.name LIKE '%" + inputs.getSampleName() + "%'");
      }

      // If the user has selected the "My data only" button.
      // (E.g. they want to display or search through all the data they have access to.
      if (inputs.getMyData())
      {
        joinClause.push_back("JOIN inves.investigationUsers users");
        joinClause.push_back("JOIN users.user user");
        whereClause.push_back("user.name = :user");
      }

      // Investigators complete name.
      if (!inputs.getInvestigatorSurName().empty())
      {
        // We join another investigationUsers & user tables as we need two aliases.
        joinClause.push_back("JOIN inves.investigationUsers usrs");
        joinClause.push_back("JOIN usrs.user usr");
        whereClause.push_back("usr.fullName LIKE '%" + inputs.getInvestigatorSurName() + "%'");
      }

      // Similar to above. We check if either has been input,
      // join the related table and add the specific WHERE clause.
      if(!inputs.getDatafileName().empty() || (inputs.getRunStart() > 0 && inputs.getRunEnd() > 0))
      {
        joinClause.push_back("JOIN inves.datasets dataset");
        joinClause.push_back("JOIN dataset.datafiles datafile");

        if (!inputs.getDatafileName().empty())
        {
          whereClause.push_back("datafile.name LIKE '%" + inputs.getDatafileName() + "%'");
        }

        if (inputs.getRunStart() > 0 && inputs.getRunEnd() > 0)
        {
          joinClause.push_back("JOIN datafile.parameters datafileparameters");
          joinClause.push_back("JOIN datafileparameters.type dtype");
          whereClause.push_back("dtype.name='run_number' AND datafileparameters.numericValue BETWEEN "
              + Strings::toString(inputs.getRunStart()) + " AND " + Strings::toString(inputs.getRunEnd()) + "");
        }
      }

      std::string query;

      // This prevents the user searching the entire archive (E.g. there is no "default" query).
      if (!whereClause.empty() || !joinClause.empty())
      {
        std::string select, selectC, from, join, where, orderBy, includes, limits;

        select   = "SELECT DISTINCT inves";
        selectC  = "SELECT COUNT(DISTINCT inves)";
        from     = " FROM Investigation inves ";
        join     = Strings::join(joinClause.begin(), joinClause.end(), " ");
        where    = Strings::join(whereClause.begin(), whereClause.end(), " AND ");
        orderBy  = " ORDER BY inves.id DESC";
        includes = " INCLUDE inves.investigationInstruments.instrument, inves.parameters";
        limits   = " LIMIT " + boost::lexical_cast<std::string>(offset) + "," + boost::lexical_cast<std::string>(limit);

        // As we joined all WHERE claused with AND we need to include the WHERE at the start of the where segment.
        where.insert(0, " WHERE ");
        // This query is used to populate investigation table on GUI.
        query    = select + from + join + where + orderBy + includes + limits;
        // This query is used to count the number of investigations returned.
        m_countQuery = selectC + from + join + where + orderBy + includes;
      }

      g_log.debug() << "ICat4Catalog::getSearchQuery: { " << query << " }" << std::endl;

      return (query);
    }

    /**
     * Searches for the relevant data based on user input.
     * @param inputs   :: reference to a class contains search inputs
     * @param outputws :: shared pointer to search results workspace
     * @param offset   :: skip this many rows and start returning rows from this point.
     * @param limit    :: limit the number of rows returned by the query.
     */
    void ICat4Catalog::search(const CatalogSearchParam& inputs, Mantid::API::ITableWorkspace_sptr& outputws,
        const int &offset, const int &limit)
    {
      // Obtain the query from user input.
      std::string query = getSearchQuery(inputs,offset,limit);

      if (query.empty()) throw std::runtime_error("You have not input any terms to search for.");

      // If offset or limit is default value then we want to return as
      // we just wanted to perform the getSearchQuery to build our COUNT query.
      if (offset == -1 || limit == -1) return;

      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;
      request.query = &query;

      int result = icat.search(&request, &response);

      if (result == 0)
      {
        saveInvestigations(response.return_, outputws);
      }
      else
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Modifies the search query to obtain the number
     * of investigations to be returned by the catalog.
     * @return The number of investigations returned by the search performed.
     */
    long ICat4Catalog::getNumberOfSearchResults()
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;
      request.query         = &m_countQuery;

      g_log.debug() << "ICat4Catalog::getNumberOfSearchResults -> Query is: { " << m_countQuery << " }" << std::endl;

      int result = icat.search(&request, &response);

      xsd__long * numOfResults;
      if (result == 0)
      {
        numOfResults = dynamic_cast<xsd__long*>(response.return_.at(0));
      }
      else
      {
        throwErrorMessage(icat);
      }

      g_log.debug() << "ICat4Catalog::getNumberOfSearchResults -> Number of results returned is: { " << numOfResults->__item << " }" << std::endl;

      return numOfResults->__item;
    }

    /**
     * Returns the logged in user's investigations data.
     * @param outputws :: Pointer to table workspace that stores the data.
     */
    void ICat4Catalog::myData(Mantid::API::ITableWorkspace_sptr& outputws)
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::string query = "Investigation INCLUDE InvestigationInstrument, Instrument, InvestigationParameter <-> InvestigationUser <-> User[name = :user]";
      request.query     = &query;

      int result = icat.search(&request, &response);

      if (result == 0)
      {
        saveInvestigations(response.return_, outputws);
      }
      else
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Saves investigations to a table workspace.
     * @param response :: A vector containing the results of the search query.
     * @param outputws :: Shared pointer to output workspace.
     */
    void ICat4Catalog::saveInvestigations(std::vector<xsd__anyType*> response, API::ITableWorkspace_sptr& outputws)
    {
      // Add rows headers to the output workspace.
      outputws->addColumn("long64","Investigation id");
      outputws->addColumn("str","Title");
      outputws->addColumn("str","Instrument");
      outputws->addColumn("str","Run range");
      outputws->addColumn("str","Start date");
      outputws->addColumn("str","End date");

      // Add data to each row in the output workspace.
      std::vector<xsd__anyType*>::const_iterator iter;
      for(iter = response.begin(); iter != response.end(); ++iter)
      {
        // Cast from xsd__anyType to subclass (xsd__string).
        ns1__investigation * investigation = dynamic_cast<ns1__investigation*>(*iter);
        if (investigation)
        {
          // Now attempt to add the relevant data to the output workspace.
          try
          {
            API::TableRow table = outputws->appendRow();
            // Used to insert an empty string into the cell if value does not exist.
            std::string emptyCell("");

            // Now add the relevant investigation data to the table (They always exist).
            savetoTableWorkspace(investigation->id, table);
            savetoTableWorkspace(investigation->title, table);
            savetoTableWorkspace(investigation->investigationInstruments.at(0)->instrument->name, table);

            // Verify that the run parameters vector exist prior to doing anything.
            // Since some investigations may not have run parameters.
            if (!investigation->parameters.empty())
            {
              savetoTableWorkspace(investigation->parameters[0]->stringValue, table);
            }
            else
            {
              savetoTableWorkspace(&emptyCell, table);
            }

            // Again, we need to check first if start and end date exist prior to insertion.
            if (investigation->startDate)
            {
              std::string startDate = formatDateTime(*investigation->startDate, "%Y-%m-%d");
              savetoTableWorkspace(&startDate, table);
            }
            else
            {
              savetoTableWorkspace(&emptyCell, table);
            }

            if (investigation->endDate)
            {
              std::string endDate = formatDateTime(*investigation->endDate, "%Y-%m-%d");
              savetoTableWorkspace(&endDate, table);
            }
          }
          catch(std::runtime_error&)
          {
            throw;
          }
        }
        else
        {
          throw std::runtime_error("ICat4Catalog::saveInvestigations expected an investigation. Please contact the Mantid development team.");
        }
      }
    }

    /**
     * Returns the datasets associated to the given investigation id.
     * @param investigationId :: unique identifier of the investigation
     * @param outputws        :: shared pointer to datasets
     */
    void ICat4Catalog::getDataSets(const long long& investigationId, Mantid::API::ITableWorkspace_sptr& outputws)
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::string query = "Datafile <-> Dataset <-> Investigation[id = '" + boost::lexical_cast<std::string>(investigationId) + "']";
      request.query     = &query;

      g_log.debug() << "ICat4Catalog::getDataSets -> { " << query << " }" << std::endl;

      int result = icat.search(&request, &response);

      if (result == 0)
      {
        saveDataSets(response.return_, outputws);
      }
      else
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Loops through the response vector and saves the datasets details to a table workspace.
     * @param response :: A vector containing the results of the search query.
     * @param outputws :: Shared pointer to output workspace.
     */
    void ICat4Catalog::saveDataSets(std::vector<xsd__anyType*> response, API::ITableWorkspace_sptr& outputws)
    {
      // Add rows headers to the output workspace.
      outputws->addColumn("str","Name");
      outputws->addColumn("str","Status");
      outputws->addColumn("str","Type");
      outputws->addColumn("str","Description");
      outputws->addColumn("str","Sample Id");
      std::string temp("");

      std::vector<xsd__anyType*>::const_iterator iter;
      for(iter = response.begin(); iter != response.end(); ++iter)
      {
        try
        {
          API::TableRow table = outputws->appendRow();
          // These are just temporary values in order for the GUI to not die.
          // These along with related GUI aspects will be removed in another ticket.
          savetoTableWorkspace(&temp, table);
          savetoTableWorkspace(&temp, table);
          savetoTableWorkspace(&temp, table);
          savetoTableWorkspace(&temp, table);
          savetoTableWorkspace(&temp, table);
        }
        catch(std::runtime_error&)
        {
          throw;
        }
      }
    }

    /**
     * Returns the datafiles associated to the given investigation id.
     * @param investigationId  :: unique identifier of the investigation
     * @param outputws         :: shared pointer to datasets
     */
    void ICat4Catalog::getDataFiles(const long long& investigationId, Mantid::API::ITableWorkspace_sptr& outputws)
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::string query = "Datafile <-> Dataset <-> Investigation[id = '" + boost::lexical_cast<std::string>(investigationId) + "']";
      request.query     = &query;

      g_log.debug() << "ICat4Catalog::getDataSets -> { " << query << " }" << std::endl;

      int result = icat.search(&request, &response);

      if (result == 0)
      {
        saveDataFiles(response.return_, outputws);
      }
      else
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Saves result from "getDataFiles" to workspace.
     * @param response :: result response from the catalog.
     * @param outputws :: shared pointer to datasets
     */
    void ICat4Catalog::saveDataFiles(std::vector<xsd__anyType*> response, API::ITableWorkspace_sptr& outputws)
    {
      // Add rows headers to the output workspace.
      outputws->addColumn("str","Name");
      outputws->addColumn("str","Location");
      outputws->addColumn("str","Create Time");
      outputws->addColumn("long64","Id");
      outputws->addColumn("str","File size");

      std::vector<xsd__anyType*>::const_iterator iter;
      for(iter = response.begin(); iter != response.end(); ++iter)
      {
        ns1__datafile * datafile = dynamic_cast<ns1__datafile*>(*iter);
        if (datafile)
        {
          try
          {
            API::TableRow table = outputws->appendRow();
            // Now add the relevant investigation data to the table.
            savetoTableWorkspace(datafile->name, table);
            savetoTableWorkspace(datafile->location, table);

            std::string createDate = formatDateTime(*datafile->createTime, "%Y-%m-%d %H:%M:%S");
            savetoTableWorkspace(&createDate, table);

            savetoTableWorkspace(datafile->id, table);
            std::string fileSize = bytesToString(*datafile->fileSize);
            savetoTableWorkspace(&fileSize, table);
          }
          catch(std::runtime_error&)
          {
            throw;
          }
        }
        else
        {
          throw std::runtime_error("ICat4Catalog::saveDataFiles expected a datafile. Please contact the Mantid development team.");
        }
      }
    }

    /**
     * Returns the list of instruments.
     * @param instruments :: instruments list
     */
    void ICat4Catalog::listInstruments(std::vector<std::string>& instruments)
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::string query = "Instrument.name ORDER BY name";
      request.query     = &query;

      int result = icat.search(&request, &response);

      if (result == 0)
      {
        for(unsigned i = 0; i < response.return_.size(); ++i)
        {
          xsd__string * instrument = dynamic_cast<xsd__string*>(response.return_[i]);;
          if (instrument)
          {
            instruments.push_back(instrument->__item);
          }
          else
          {
            throw std::runtime_error("ICat4Catalog::listInstruments expected an instrument. Please contact the Mantid development team.");
          }
        }
      }
      else
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Returns the list of investigation types.
     * @param invstTypes :: investigation types list
     */
    void ICat4Catalog::listInvestigationTypes(std::vector<std::string>& invstTypes)
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::string query = "InvestigationType.name ORDER BY name";
      request.query     = &query;

      int result = icat.search(&request, &response);

      if (result == 0)
      {
        for(unsigned i = 0; i < response.return_.size(); ++i)
        {
          xsd__string * investigation = dynamic_cast<xsd__string*>(response.return_[i]);
          if (investigation)
          {
            invstTypes.push_back(investigation->__item);
          }
          else
          {
            throw std::runtime_error("ICat4Catalog::listInvestigationTypes expected a string. Please contact the Mantid development team.");
          }
        }
      }
      else
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Gets the file location string from the archives.
     * @param fileID       :: id of the file
     * @param fileLocation :: location string  of the file
     */
    void ICat4Catalog::getFileLocation(const long long & fileID, std::string & fileLocation)
    {
      ICat4::ICATPortBindingProxy icat;
      setSSLContext(icat);

      ns1__get request;
      ns1__getResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::string query     = "Datafile";
      request.query         = &query;
      request.primaryKey    = fileID;

      int result = icat.get(&request, &response);

      if (result == 0)
      {
        ns1__datafile * datafile = dynamic_cast<ns1__datafile*>(response.return_);

        if (datafile)
        {
          if(datafile->location)
          {
            fileLocation = *(datafile->location);
          }
        }
        else
        {
          throw std::runtime_error("ICat4Catalog::getFileLocation expected a datafile. Please contact the Mantid development team.");
        }
      }
      else
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Downloads a file from the given url if not downloaded from archive.
     * @param fileID :: id of the file
     * @param url    :: url of the file
     */
    void ICat4Catalog::getDownloadURL(const long long & fileID, std::string& url)
    {
      // Obtain the URL from the Facilities.xml file.
      std::string urlToBuild = ConfigService::Instance().getFacility().catalogInfo().externalDownloadURL();

      // Set the REST features of the URL.
      std::string session  = "sessionId="    + Session::Instance().getSessionId();
      std::string datafile = "&datafileIds=" + boost::lexical_cast<std::string>(fileID);
      std::string outname  = "&outname="     + boost::lexical_cast<std::string>(fileID);

      // Add all the REST pieces to the URL.
      urlToBuild += ("getData?" + session + datafile + outname + "&zip=false");

      g_log.debug() << "ICat4Catalog::getDownloadURL -> { " << urlToBuild << " }" << std::endl;

      url = urlToBuild;
    }

    /**
     * Keep the current session alive.
     */
    void ICat4Catalog::keepAlive()
    {
    }

    /**
     * Keep the current session alive in minutes.
     * @return The number of minutes to keep session alive for.
     */
    int ICat4Catalog::keepAliveinminutes()
    {
      return (0);
    }

    /**
     * Defines the SSL authentication scheme.
     * @param icat :: ICATPortBindingProxy object.
     */
    void ICat4Catalog::setSSLContext(ICat4::ICATPortBindingProxy& icat)
    {
      if (soap_ssl_client_context(&icat,
          SOAP_SSL_CLIENT, /* use SOAP_SSL_DEFAULT in production code */
          NULL,       /* keyfile: required only when client must authenticate to
              server (see SSL docs on how to obtain this file) */
          NULL,       /* password to read the keyfile */
          NULL,      /* optional cacert file to store trusted certificates */
          NULL,      /* optional capath to directory with trusted certificates */
          NULL      /* if randfile!=NULL: use a file with random data to seed randomness */
      ))
      {
        throwErrorMessage(icat);
      }
    }

    /**
     * Throws an error message (returned by gsoap) to Mantid upper layer.
     * @param icat :: ICATPortBindingProxy object.
     */
    void ICat4Catalog::throwErrorMessage(ICat4::ICATPortBindingProxy& icat)
    {
      char buf[600];
      const int len = 600;
      icat.soap_sprint_fault(buf,len);
      std::string error(buf);
      std::string begmsg("<message>");
      std::string endmsg("</message>");

      std::basic_string <char>::size_type start = error.find(begmsg);
      std::basic_string <char>::size_type end   = error.find(endmsg);
      std::string exception;

      if(start != std::string::npos && end != std::string::npos)
      {
        exception = error.substr(start + begmsg.length(), end - (start + begmsg.length()) );
      }

      throw std::runtime_error(exception);
    }

    /**
     * Convert a file size to human readable file format.
     * @param fileSize :: The size in bytes of the file.
     */
    std::string ICat4Catalog::bytesToString(int64_t &fileSize)
    {
      const char* args[] = {"B", "KB", "MB", "GB"};
      std::vector<std::string> units(args, args + 4);

      unsigned order = 0;

      while (fileSize >= 1024 && order + 1 < units.size())
      {
          order++;
          fileSize = fileSize / 1024;
      }

      return boost::lexical_cast<std::string>(fileSize) + units.at(order);
    }

    /**
     * Formats a given timestamp to human readable datetime.
     * @param timestamp :: Unix timestamp.
     * @param format    :: The desired format to output.
     * @return string   :: Formatted Unix timestamp.
     */
    std::string ICat4Catalog::formatDateTime(const time_t &timestamp, const std::string &format)
    {
      auto dateTime = DateAndTime(boost::posix_time::from_time_t(timestamp));
      return (dateTime.toFormattedString(format));
    }

  }
}
