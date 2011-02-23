#ifndef MANTID_ICAT_SEARCHHELPER_H_
#define MANTID_ICAT_SEARCHHELPER_H_


#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Logger.h"
#include "MantidICat/SearchParam.h"


namespace Mantid
{
	namespace ICat
	{
/**  CSearchHelper is a utility class used in Mantid ICat3 based information catalog 
     class to connect  ICat API services using the gsoap generated proxy class and retrieve data from ICat services.
     
     @author Sofia Antony, ISIS Rutherford Appleton Laboratory 
     @date 07/07/2010
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

		class CICatHelper
		{
		public:
			
			/// constructor
			CICatHelper():g_log(Kernel::Logger::get("CICatHelper"))
			{}
			/// destructor
			~CICatHelper(){}
			
			/// search method
			int doSearch(ICATPortBindingProxy& icat,boost::shared_ptr<ns1__searchByAdvanced>& request,ns1__searchByAdvancedResponse& response);
		
			/// method to search isis basic search
			void doISISSearch(const CSearchParam& input,API::ITableWorkspace_sptr &outputws);

			/// calls getInvestigationIncludes api's
			int getDataFiles(long long invId,ns1__investigationInclude inclide,API::ITableWorkspace_sptr& responsews_sptr);

			/// this method calls Icat api getInvestigationIncludes and returns datasets for the given investigation id.
			int doDataSetsSearch(long long invId,ns1__investigationInclude inclide,API::ITableWorkspace_sptr& responsews_sptr);

			/// This method lists the isntruments
			void  listInstruments(std::vector<std::string>& instruments);

			/// This method lists the investigation types
			void listInvestigationTypes(std::vector<std::string>& investTypes);

			/// This method disconnects last connected  session from icat DB
			int doLogout();

			/// This method does investigations data search for logged in user
			void doMyDataSearch(API::ITableWorkspace_sptr& ws_sptr);

      /// do advanced search 
			void doAdvancedSearch(CSearchParam& inputs,API::ITableWorkspace_sptr &outputws);

			// do login
			void doLogin(const std::string& name,const std::string& password,const std::string& url);

			
			/// Thsi method returns the time_t value for a Date which is in "DD/MM/YYYY" format
			time_t getTimevalue(const std::string& sDate);

			/// thsi method returns true if the  session id is valid
			bool isvalidSession();

			/// get the url of the given file id
			void getdownloadURL(const long long& fileId,std::string& url);
			
			/// get location of data file  or download method
			void  getlocationString(const long long& fileid,std::string& filelocation);

							
		private:
			
			/// This method sets the request parameters for investigation includes.
			void setReqParamforInvestigationIncludes(long long invstId,ns1__investigationInclude include,
				ns1__getInvestigationIncludes& request);

			///This method saves the file search response to table workspace
			API::ITableWorkspace_sptr saveFileSearchResponse(const ns1__searchByAdvancedResponse& response);

			/// This method saves the response data of search by run number to table workspace
			void saveSearchRessults(const ns1__searchByAdvancedResponse& response,API::ITableWorkspace_sptr& outputws);

			/// this method saves investigation include response to a table workspace
			void  saveInvestigationIncludesResponse(
				const ns1__getInvestigationIncludesResponse& response,
				API::ITableWorkspace_sptr& outputws);

			/// This method saves Datasets to a table workspace
			void  saveDataSets(const ns1__getInvestigationIncludesResponse& response,API::ITableWorkspace_sptr& outputws);

			/// This method sets the request parameters
			void setReqparamforlistInstruments(ns1__listInstruments& request);

				
			/// This method creates table workspace
			API::ITableWorkspace_sptr createTableWorkspace();
			
			/// This method checks the given file name is raw file or nexus file
			bool isDataFile(const std::string* fileName);

			/// This method saves the myinvestigations data to a table workspace
			void saveMyInvestigations(const ns1__getMyInvestigationsIncludesResponse& response,API::ITableWorkspace_sptr& outputws);

			///save investigations
			void saveInvestigations(const std::vector<ns1__investigation*>& investigations,API::ITableWorkspace_sptr& outputws);

			///saves 
			void saveInvestigatorsNameandSample(ns1__investigation* investigation,API::TableRow& t);
             

			/** This is a template method to save data to table workspace
			 * @param input :: pointer to input value
			 * @param t :: table row reference
       */
			template<class T>
			void savetoTableWorkspace(const T* input,Mantid::API::TableRow &t)
			{
				if(input!=0)
				{
					t<<*input;
					
				}
				else
				{
					t<<"";
				}
			}
			private:
			Kernel::Logger& g_log;    ///< reference to the logger class
		};

	
			
	}
}
#endif
