#ifndef MANTID_ICAT_SEARCHHELPER_H_
#define MANTID_ICAT_SEARCHHELPER_H_


#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Logger.h"

/**  CSearchHelper is a utility class used in Mantid-ICat algorithms to do ICat searching.
     
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

namespace Mantid
{
	namespace ICat
	{
		class CSearchInput
		{
		public:
			/// constructor
			CSearchInput():m_startRun(0),m_endRun(0),m_caseSensitive(false),
				m_startDate(0),m_endDate(0)
			{}
			/// Destructor
			~CSearchInput(){}
			/** This method  sets start date
             *  @param startRun start run number
			 */
			void setRunStart(const double& startRun){m_startRun=startRun;}
			/** This method  sets end date
           	 *  @param endRun end run number
			 */
			void setRunEnd(const double& endRun){m_endRun=endRun;}
			/** This method  sets isntrument name
           	 *  @param instrName name of the instrument
			 */
			void setInstrument(const std::string& instrName){m_instrName=instrName;}
			/** This method  sets the start date
           	 *  @param startDate start date for search
			 */
			void setStartDate(const time_t& startDate){m_startDate=startDate;}
			/** This method  sets the end date
           	 *  @param endDate end date for search
			 */
			void setEndDate(const time_t& endDate){m_endDate=endDate;}
			/** This method  sets the CaseSensitive
           	 *  @param setCaseSensitive flag to do case sensitive  search
			 */
			void setCaseSensitive(bool bCase){m_caseSensitive=bCase;}
			/** This method  sets the InvestigationInclude
           	 *  @param include enum for selecting dat from the icat db
			 */
			void setInvestigationInclude(const ns1__investigationInclude& include){m_include=include;}

			/** This method  sets the InvestigationInclude
           	 *  @param include enum for selecting data from the icat db
			 */
			void setKeywords(const std::string& keywords){m_keywords=keywords;}

			/** This method  returns the start run number
           	 *  @param returns  run start number
			 */
			const double& getRunStart(){return m_startRun; }
			/** This method  returns the end run number
           	 *  @param returns  run end number
			 */
			const double& getRunEnd(){return m_endRun;}
			/** This method  returns the instrument name
           	 *  @param returns  instrument name
			 */
			const std::string& getInstrument(){return m_instrName;}
			/** This method  returns the start date
           	 *  @param returns  start date
			 */
			const time_t& getStartDate(){return m_startDate;}
			/** This method  returns the end date
           	 *  @param returns end date for investigations serch
			 */
			const time_t& getEndDate(){return m_endDate;}
			/** This method  returns case sensitive flag
           	 *  @param returns  case sensitive flag
			 */
			bool getCaseSensitive(){return m_caseSensitive;}
			/** This method  returns the enum for data search in icat db
           	 *  @param returns  investigation include
			 */
			const ns1__investigationInclude& getInvestigationInclude(){return m_include;}
			/** This method  returns keywords used for searching
           	 *  @param returns keywords
			 */
			const std::string& getKeywords(){return m_keywords;}
		private:
			double m_startRun;
			double m_endRun;
			std::string m_instrName;
			std::string m_keywords;
			bool m_caseSensitive;
			time_t m_startDate;
			time_t m_endDate;
			ns1__investigationInclude m_include;

		};
		class CSearchHelper
		{
		public:
			//Mantid::Kernel::Logger & CSearchHelper::g_log=Mantid::Kernel::Logger::get("CSearchHelper");
			/// constructor
			CSearchHelper():g_log(Kernel::Logger::get("CSearchHelper"))
			{}
			/// destructor
			~CSearchHelper(){}
			
			/// search method
			int doSearch(ICATPortBindingProxy& icat,boost::shared_ptr<ns1__searchByAdvanced>& request,ns1__searchByAdvancedResponse& response);
			
			
			//int doSearchByRunNumber(const double& dstartRun,const double& dendRun,bool bCase,const std::string& instrName,const time_t& startDate,const time_t& endDate, ns1__investigationInclude einclude,
			//	API::ITableWorkspace_sptr& responsews_sptr);
			/// method to search isis basic search
			int doIsisSearch(CSearchInput& input,API::ITableWorkspace_sptr& responsews_sptr);

			/// calls getInvestigationIncludes api's
			int getDataFiles(long long invId,bool bDataFiles,ns1__investigationInclude inclide,API::ITableWorkspace_sptr& responsews_sptr);

			/// this method calls Icat api getInvestigationIncludes and returns datasets for the given investigation id.
			int doDataSetsSearch(long long invId,ns1__investigationInclude inclide,API::ITableWorkspace_sptr& responsews_sptr);

			/// This method lists the isntruments
			API::ITableWorkspace_sptr listInstruments();

			/// This method disconnects last connected  session from icat DB
			int doLogout();

			/// This method does investigations data search for logged in user
			void doMyDataSearch(API::ITableWorkspace_sptr & ws_sptr);
			
			
		private:
			
			/// This method sets the request parameters for investigation includes.
			void setReqParamforInvestigationIncludes(long long invstId,ns1__investigationInclude include,
				ns1__getInvestigationIncludes& request);

			/// set request param for investigation includes
			//void setReqParamforSearchByRunNumber(const double& dstart,const double& dend,bool bCase,ns1__investigationInclude einclude,
			//	boost::shared_ptr<ns1__searchByAdvanced>& request);
			void setReqParamforSearchByRunNumber(CSearchInput& input,boost::shared_ptr<ns1__searchByAdvanced>& request);
	
			///This method saves the file search response to table workspace
			API::ITableWorkspace_sptr saveFileSearchResponse(const ns1__searchByAdvancedResponse& response);

			/// This method saves the response data of search by run number to table workspace
			void saveSearchResponse(const ns1__searchByAdvancedResponse& response,API::ITableWorkspace_sptr& outputws);

			/// this method saves investigation include response to a table workspace
			void  saveInvestigationIncludesResponse(bool bDataFiles,
				const ns1__getInvestigationIncludesResponse& response,
				API::ITableWorkspace_sptr& outputws);

			/// This method saves Datasets to a table workspace
			void  saveDataSets(const ns1__getInvestigationIncludesResponse& response,API::ITableWorkspace_sptr& outputws);

			/// This method sets the request parameters
			void setReqparamforlistInstruments(ns1__listInstruments& request);

			/// This method saves Instrument List to a table workspace
			API::ITableWorkspace_sptr saveInstrumentList(const ns1__listInstrumentsResponse& response);

			/// This method creates table workspace
			API::ITableWorkspace_sptr createTableWorkspace();
			
			/// This method checks the given file name is raw file or nexus file
			bool isDataFile(const std::string* fileName);

			/// This method saves the myinvestigations data to a table workspace
			void saveMyInvestigations(const ns1__getMyInvestigationsIncludesResponse& response,API::ITableWorkspace_sptr& outputws);

			///save investigations
			void saveInvestigations(const std::vector<ns1__investigation*>& investigations,API::ITableWorkspace_sptr& outputws);
		       

			/* This is a template method to save data to table workspace
			 * @param input pointer to input value
			 * @param t table row reference
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
			Kernel::Logger& g_log;    ///< reference to the logger class
		};
	
			
	}
}
#endif
