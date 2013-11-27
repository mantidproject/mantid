#include "MantidICat/ICat3/ICat3Catalog.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidICat/Session.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid
{
  namespace ICat
  {

    DECLARE_CATALOG(ICat3Catalog)

    /// constructor
    ICat3Catalog::ICat3Catalog() : m_helper(new CICatHelper())
    {
    }
    /// destructor
    ICat3Catalog::~ICat3Catalog()
    {
    }
    /**This method is responsible for connecting the client application to ICat3 based catalog services
     *@param username :: login name(eg. federal id) of the user
     *@param password :: passowrd of the user
     *@param url :: url of the user
     */
    void ICat3Catalog::login(const std::string& username,const std::string& password,const std::string& url)
    {
      m_helper->doLogin(username,password,url);
    }
    /// This method disconnects the client application from ICat3 based catalog services
    void ICat3Catalog::logout()
    {
      m_helper->doLogout();
      Session::Instance().setSessionId("");//clearing the session id saved to Mantid after log out
    }

    /*This method returns the logged in user's investigations data .
     *@param mydataws_sptr :: pointer to table workspace which stores the data
     */
    void  ICat3Catalog::myData(Mantid::API::ITableWorkspace_sptr& mydataws_sptr)
    {
      m_helper->doMyDataSearch(mydataws_sptr);
    }

    /*This method returns  the datasets associated to the given investigationid .
     *@param investigationId :: unique identifier of the investigation
     *@param datasetsws_sptr :: shared pointer to datasets
     */
    void ICat3Catalog::getDataSets(const long long& investigationId,Mantid::API::ITableWorkspace_sptr& datasetsws_sptr)
    {
      //search datasets for a given investigation id using ICat api.
      m_helper->doDataSetsSearch(investigationId,
          ICat3::ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATASET_USCOREPARAMETERS_USCOREONLY,datasetsws_sptr);
    }

    /*This method returns the datafiles associated to the given investigationid .
     *@param investigationId :: unique identifier of the investigation
     *@param datafilesws_sptr :: shared pointer to datasets
     */
    void ICat3Catalog::getDataFiles(const long long& investigationId,Mantid::API::ITableWorkspace_sptr& datafilesws_sptr)
    {
      m_helper->getDataFiles(investigationId,ICat3::ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,datafilesws_sptr);
    }

    /**This method returns the list of instruments
     *@param instruments :: instruments list
     */
    void ICat3Catalog::listInstruments(std::vector<std::string>& instruments)
    {
      m_helper->listInstruments(instruments);
    }

    /**This method returns the list of investigationtypes
     *@param invstTypes :: investigation types list
     */
    void  ICat3Catalog::listInvestigationTypes(std::vector<std::string>& invstTypes)
    {
      m_helper->listInvestigationTypes(invstTypes);
    }

    /**This method method gets the file location strings from isis archive
     *@param fileid :: id of the file
     *@param filelocation :: location string  of the file
     */
    void ICat3Catalog::getFileLocation(const long long & fileid,std::string & filelocation)
    {
      m_helper->getlocationString(fileid,filelocation);
    }

    /**This method method gets the url for downloading the file from isis server
     *@param fileid :: id of the file
     *@param url :: url  of the file
     */
    void ICat3Catalog::getDownloadURL(const long long & fileid,std::string& url)
    {
      m_helper->getdownloadURL(fileid,url);
    }

    /**This method method does the search for investigations
     *@param inputs :: reference to a class conatains search inputs
     *@param ws_sptr :: -shared pointer to search results workspace
     *@param offset  :: skip this many rows and start returning rows from this point.
     *@param limit   :: limit the number of rows returned by the query.
     */
    void ICat3Catalog::search(const CatalogSearchParam& inputs, Mantid::API::ITableWorkspace_sptr& ws_sptr,
        const int &offset, const int &limit)
    {
      m_helper->doAdvancedSearch(inputs,ws_sptr, offset, limit);
    }

    /**
     * Modifies the search query to obtain the number
     * of investigations to be returned by the catalog.
     * @return The number of investigations returned by the search performed.
     */
    int64_t ICat3Catalog::getNumberOfSearchResults(const CatalogSearchParam& inputs)
    {
      return m_helper->getNumberOfSearchResults(inputs);
    }

    /// keep alive
    void ICat3Catalog::keepAlive()
    {
    }
    //keep alive in minutes
    int ICat3Catalog::keepAliveinminutes()
    {
      return 0;
    }


  }
}
