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

 /**This class is responsible for the implementation of ICat3 version based information catalogs .
    @author Sofia Antony, ISIS Rutherford Appleton Laboratory 
    @date 20/10/2010
    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    
	*/	

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
