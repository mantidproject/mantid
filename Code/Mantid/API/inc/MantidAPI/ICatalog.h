#ifndef MANTID_API_ICATLOG_H_
#define MANTID_API_ICATLOG_H_
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid
{
	//forward declarations
   namespace ICat
   {
	 
     class CSearchParam;
    }
	namespace API
	{  
		
		class DLLExport ICatalog
		{
		public:
			// virtual destructor
			virtual ~ICatalog(){};
			/// method to login to a catalog
			virtual void login(const std::string&,const std::string&,const std::string&)=0;
			/// logout from catalog
			virtual void logout()=0;
			///Search investigations 
			virtual void search(const ICat::CSearchParam&,ITableWorkspace_sptr &)=0;
			/// search logged in users data
			virtual void myData(ITableWorkspace_sptr &)=0;
			/// get datasets.
			virtual void getDataSets(const long long& ,ITableWorkspace_sptr&)=0;
			/// get datafiles
			virtual void getDataFiles(const long long&,ITableWorkspace_sptr &)=0;
			///  instrument list
			virtual void listInstruments(std::vector<std::string>& )=0;
			/// get investigationtype lists
			virtual void listInvestigationTypes(std::vector<std::string>&)=0;
			/// get file locations
			virtual void getFileLocation(const long long&,std::string& )=0;
			/// get URLs of the files
			virtual void getDownloadURL(const long long& fileid,std::string&)=0;
			/// keep alive
			virtual void keepAlive()=0;
			//keep alive in minutes
			virtual int keepAliveinminutes()=0; 


		};

		typedef boost::shared_ptr<ICatalog> ICatalog_sptr;
		typedef boost::shared_ptr<const ICatalog> ICatalog_const_sptr;
	}
}
#endif