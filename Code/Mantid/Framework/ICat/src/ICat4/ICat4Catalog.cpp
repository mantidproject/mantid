#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/Progress.h"
#include "MantidICat/ICat4/GSoapGenerated/ICat4ICATPortBindingProxy.h"
#include "MantidICat/ICat4/ICat4Catalog.h"
#include "MantidICat/Session.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/DateAndTime.h"

namespace Mantid
{
  namespace ICat
  {
    using namespace ICat4;
    using namespace Kernel;

    DECLARE_CATALOG(ICat4Catalog)

    /// constructor
    ICat4Catalog::ICat4Catalog() { }

    /// destructor
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
      ICATPortBindingProxy icat;

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
        throwErrorMessage(icat);
      }
    }

    /**
     * Disconnects the client application from ICat4 based catalog services.
     */
    void ICat4Catalog::logout()
    {
      ICATPortBindingProxy icat;
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
        throw std::runtime_error("You are not currently logged into the system.");
      }
    }

    /**
     * Creates a search query string based on inputs provided by the user.
     * @param inputs :: reference to a class contains search inputs.
     * @return a query string constructed from user input.
     */
    std::string ICat4Catalog::getSearchQuery(const CatalogSearchParam& inputs)
    {
      // This will hold strings for each table of the query. Each segment will be joined together (<->).
      std::vector<std::string> querySegments;

      // The investigation segment will be stored here as it makes up for several inputs.
      // It will be converted to a string, joined and then added to the querySegments.
      std::vector<std::string> investigationWhere;

      // In ICat4.2 `Dataset` and `Sample` cannot be in the same query (due to restriction with join).
      // As such, we will query for sample only when dataset inputs are not used.
      bool queryDataset = false;

      // Investigation Start and end date
      if(inputs.getStartDate() != 0 && inputs.getEndDate() != 0)
      {
        // Format the timestamps in order to compare them.
        std::string startDate = formatDateTime(inputs.getStartDate());
        std::string endDate   = formatDateTime(inputs.getEndDate());
        // Make it so...
        investigationWhere.push_back("startDate >= '" + startDate + "' AND startDate <= '" + endDate + "' OR endDate >= '" + startDate + "' AND endDate <= '" + endDate + "'");
      }

      // Investigation name (title)
      if(!inputs.getInvestigationName().empty())
      {
        investigationWhere.push_back("title LIKE '%" + inputs.getInvestigationName() + "%' ");
      }

      // Investigation abstract
      if(!inputs.getInvestigationAbstract().empty())
      {
        investigationWhere.push_back("summary = '" + inputs.getInvestigationAbstract() + "' ");
      }

      // Iterate over query vector and append AND between inputs.
      std::string investigationResult = Strings::join(investigationWhere.begin(), investigationWhere.end(), " AND ");

      // Add the investigation result to the query if it exists.
      if (!investigationResult.empty())
      {
        querySegments.push_back("Investigation[" + investigationResult + "]");
      }

      // Investigation type
      if(!inputs.getInvestigationType().empty())
      {
        querySegments.push_back("InvestigationType[name IN ('" + inputs.getInvestigationType() + "')]");
      }

      // Investigator's surname
      if(!inputs.getInvestigatorSurName().empty())
      {
        querySegments.push_back("InvestigationUser <-> User[name LIKE '%" + inputs.getInvestigatorSurName() + "%']");
      }

      // Datafile name
      if(!inputs.getDatafileName().empty())
      {
        querySegments.push_back("Dataset <-> Datafile[name = '" + inputs.getDatafileName() + "']");
        queryDataset = true;
      }

      // Run start and end
      if(inputs.getRunStart() > 0 && inputs.getRunEnd() > 0)
      {
        // Convert the start and end runs to string.
        std::string runStart = Strings::toString(inputs.getRunStart());
        std::string runEnd   = Strings::toString(inputs.getRunEnd());

        // To be able to use DatafileParameter we need to have access to Dataset and Datafile.
        // If queryDataset is true, then we can rest assured that the relevant access is possible.
        if (queryDataset)
        {
          querySegments.push_back("DatafileParameter[type.name ='run_number' AND numericValue BETWEEN " + runStart + " AND " + runEnd + "]");
        }
        else
        {
          // Otherwise we directly include them ourselves.
          querySegments.push_back("Dataset <-> Datafile <-> DatafileParameter[type.name ='run_number' AND numericValue BETWEEN " + runStart + " AND " + runEnd + "]");
          // We then set queryDataset to true since Sample can not be included if a dataset is.
          queryDataset = true;
        }
      }

      // Instrument name
      if(!inputs.getInstrument().empty())
      {
        querySegments.push_back("Instrument[name = '" + inputs.getInstrument() + "']");
      }

      // Keywords
      if(!inputs.getKeywords().empty())
      {
        querySegments.push_back("Keyword[name IN ('" + inputs.getKeywords() + "')]");
      }

      // Sample name
      if(!inputs.getSampleName().empty() && !queryDataset)
      {
        querySegments.push_back("Sample[name = '" + inputs.getSampleName() + "']");
      }

      // Now we build the query from the segments. For each segment, we append a join ("<->").
      std::string query = Strings::join(querySegments.begin(), querySegments.end(), " <-> ");

      // We then append the required includes to output related data, such as instrument name and run parameters.
      if (!query.empty())
      {
        query.insert(0, "DISTINCT Investigation INCLUDE Instrument, InvestigationParameter <-> ");
      }

      return (query);
    }

    /**
     * Searches for the relevant data based on user input.
     * @param inputs   :: reference to a class contains search inputs
     * @param outputws :: shared pointer to search results workspace
     */
    void ICat4Catalog::search(const CatalogSearchParam& inputs, Mantid::API::ITableWorkspace_sptr& outputws)
    {
    }

    /**
     * Returns the logged in user's investigations data.
     * @param outputws :: Pointer to table workspace that stores the data.
     */
    void ICat4Catalog::myData(Mantid::API::ITableWorkspace_sptr& outputws)
    {
      ICATPortBindingProxy icat;

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::string query = "Investigation INCLUDE Instrument, InvestigationParameter <-> InvestigationUser <-> User[name = :user]";
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
      outputws->addColumn("str","Investigation Number");
      outputws->addColumn("str","Title");
      outputws->addColumn("str","Instrument");
      outputws->addColumn("str","Run Range");

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
            // Now add the relevant investigation data to the table.
            savetoTableWorkspace(investigation->name, table); // Investigation number
            savetoTableWorkspace(investigation->title, table);
            savetoTableWorkspace(investigation->instrument->name, table);
            // Verify that the run parameters vector exist prior to doing anything.
            // Since some investigations may not have run parameters.
            if (!investigation->parameters.empty())
            {
              savetoTableWorkspace(investigation->parameters[0]->stringValue, table);
            }
          }
          catch(std::runtime_error&)
          {
            throw std::runtime_error("An error occurred when saving the ICat search results data to Workspace");
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
    }

    /**
     * Returns the datafiles associated to the given investigation id.
     * @param investigationId  :: unique identifier of the investigation
     * @param outputws         :: shared pointer to datasets
     */
    void ICat4Catalog::getDataFiles(const long long& investigationId, Mantid::API::ITableWorkspace_sptr& outputws)
    {
      ICATPortBindingProxy icat;

      ns1__search request;
      ns1__searchResponse response;

      std::string sessionID = Session::Instance().getSessionId();
      request.sessionId     = &sessionID;

      std::ostringstream temp;
      temp << investigationId;
      std::string name = temp.str();

      std::string query = "Datafile <-> Dataset <-> Investigation[name = '" + name + "']";
      request.query     = &query;

      // If the investigation name is not valid.
      if(name == "0" || name.empty())
      {
        throw std::runtime_error("Invalid investigation ID supplied.");
      }

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
     * @param investigationId :: unique identifier of the investigation
     * @param outputws        :: shared pointer to datasets
     */
    void ICat4Catalog::saveDataFiles(std::vector<xsd__anyType*> response, API::ITableWorkspace_sptr& outputws)
    {
      // Add rows headers to the output workspace.
      outputws->addColumn("str","Name");
      outputws->addColumn("str","Location");
      outputws->addColumn("long64","Id");
      outputws->addColumn("str","Create Time");

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
            savetoTableWorkspace(datafile->id, table);

            std::string createDate = formatDateTime(*(datafile->createTime));
            savetoTableWorkspace(&createDate, table);
          }
          catch(std::runtime_error&)
          {
            throw std::runtime_error("An error occurred when saving file data to workspace.");
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
      ICATPortBindingProxy icat;

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
      ICATPortBindingProxy icat;

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
     * @param fileid       :: id of the file
     * @param filelocation :: location string  of the file
     */
    void ICat4Catalog::getFileLocation(const long long & fileid, std::string & filelocation)
    {
    }

    /**
     * Downloads a file from the given url.
     * @param fileid :: id of the file
     * @param url    :: url  of the file
     */
    void ICat4Catalog::getDownloadURL(const long long & fileid, std::string& url)
    {
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
    void ICat4Catalog::setSSLContext(ICATPortBindingProxy& icat)
    {
      if (soap_ssl_client_context(&icat,
          SOAP_SSL_NO_AUTHENTICATION, /* use SOAP_SSL_DEFAULT in production code */
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
    void ICat4Catalog::throwErrorMessage(ICATPortBindingProxy& icat)
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
     * Formats a given timestamp to human readable datetime.
     * @param timestamp :: Unix timestamp.
     * @return string   :: Formatted Unix timestamp in the format "%Y-%b-%d %H:%M:%S"
     */
    std::string ICat4Catalog::formatDateTime(time_t timestamp)
    {
      auto dateTime = DateAndTime(boost::posix_time::from_time_t(timestamp));
      return (dateTime.toFormattedString());
    }

  }
}
