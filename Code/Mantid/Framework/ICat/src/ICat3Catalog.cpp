#include "MantidICat/ICat3Catalog.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidICat/Session.h"
#include "MantidAPI/Progress.h"
#include "MantidICat/ICatHelper.h"

namespace Mantid
{
	namespace ICat
	{

		DECLARE_CATALOG(ICat3Catalog)

		/// constructor
		ICat3Catalog::ICat3Catalog()
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
			CICatHelper helper;
			helper.doLogin(username,password,url);
		}
		/// This method disconnects the client application from ICat3 based catalog services
		void ICat3Catalog::logout()
		{

			CICatHelper helper;
			helper.doLogout();
			Session::Instance().setSessionId("");//clearing the session id saved to Mantid after log out
		}
		
		/*This method returns the logged in user's investigations data .
		 *@param mydataws_sptr :: pointer to table workspace which stores the data
		 */
		void  ICat3Catalog::myData(Mantid::API::ITableWorkspace_sptr& mydataws_sptr)
		{
			CICatHelper helper;
			helper.doMyDataSearch(mydataws_sptr);
		}

		/*This method returns  the datasets associated to the given investigationid .
		 *@param investigationId :: unique identifier of the investigation
		 *@param datasetsws_sptr :: shared pointer to datasets
		 */
		void ICat3Catalog::getDataSets(const long long& investigationId,Mantid::API::ITableWorkspace_sptr& datasetsws_sptr)
		{
			CICatHelper helper;
			//search datasets for a given investigation id using ICat api.
			helper.doDataSetsSearch(investigationId,
				ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATASET_USCOREPARAMETERS_USCOREONLY,datasetsws_sptr);
		}

		/*This method returns the datafiles associated to the given investigationid .
		 *@param investigationId :: unique identifier of the investigation
	     *@param datafilesws_sptr :: shared pointer to datasets
	     */
		void ICat3Catalog::getDataFiles(const long long& investigationId,Mantid::API::ITableWorkspace_sptr& datafilesws_sptr)
		{
			CICatHelper helperobj;	
			helperobj.getDataFiles(investigationId,ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,datafilesws_sptr);
		}

	 /**This method returns the list of instruments
	   *@param instruments :: instruments list
	   */
		void ICat3Catalog::listInstruments(std::vector<std::string>& instruments)
		{
			CICatHelper helper;
			helper.listInstruments(instruments);
		}

	 /**This method returns the list of investigationtypes
	   *@param invstTypes :: investigation types list
	   */
		void  ICat3Catalog::listInvestigationTypes(std::vector<std::string>& invstTypes)
		{			
			CICatHelper helper;
			helper.listInvestigationTypes(invstTypes);
		}

	 /**This method method gets the file location strings from isis archive
	   *@param fileid :: id of the file
	   *@param filelocation :: location string  of the file
	   */
		void ICat3Catalog::getFileLocation(const long long & fileid,std::string & filelocation)
		{
			CICatHelper helper;
			helper.getlocationString(fileid,filelocation);
		}

	 /**This method method gets the url for downloading the file from isis server
	   *@param fileid :: id of the file
	   *@param url :: url  of the file
	   */
		void ICat3Catalog::getDownloadURL(const long long & fileid,std::string& url)
		{
			CICatHelper helper;
			helper.getdownloadURL(fileid,url);
		}

	 /**This method method does the search for investigations
	   *@param inputs :: reference to a class conatains search inputs
	   *@param ws_sptr :: -shared pointer to search results workspace
	   */
		void ICat3Catalog::search(const CSearchParam& inputs, Mantid::API::ITableWorkspace_sptr& ws_sptr)
		{
			CICatHelper helper;
			helper.doISISSearch(inputs,ws_sptr);
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
