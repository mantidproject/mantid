#ifndef MANTID_ICAT_ICAT3CATALOG_H_
#define MANTID_ICAT_ICAT3CATALOG_H_
#include "MantidAPI/ICatalog.h"
#include "MantidICat/SearchParam.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"
#include "MantidICat/ErrorHandling.h"
namespace Mantid
{
	namespace ICat
	{

		class  ICat3Catalog : public Mantid::API::ICatalog
		{
		public:
			/// constructor
			ICat3Catalog();
			/// destructor
			virtual ~ICat3Catalog();
		/// login to isis catalog
			virtual void login(const std::string& username,const std::string& password,const std::string& url);
			///logout from isis catalog
			virtual void logout();
			/// search isis data
			virtual void search(const CSearchParam& inputs, Mantid::API::ITableWorkspace_sptr& ws_sptr);
			/// logged in user's investigations search
			virtual void myData(Mantid::API::ITableWorkspace_sptr& mydataws_sptr);
			/// get datasets
			virtual void getDataSets(const long long&investigationId,Mantid::API::ITableWorkspace_sptr& datasetsws_sptr);
			/// get datafiles
			virtual void getDataFiles(const long long&investigationId,Mantid::API::ITableWorkspace_sptr& datafilesws_sptr);
			/// get instruments list
			virtual void listInstruments(std::vector<std::string>& instruments);
			/// get investigationtypes list
			virtual void listInvestigationTypes(std::vector<std::string>& invstTypes);
			/// get file location strings
			virtual void getFileLocation(const long long&fileid,std::string& filelocation);
			/// get urls
			virtual void getDownloadURL(const long long& fileid,std::string & fileLocation);
			/// keep alive
			virtual void keepAlive();
			//keep alive in minutes
			virtual int keepAliveinminutes(); 
		
		};
	}
}

#endif