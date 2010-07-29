#ifndef MANTID_ICAT_SEARCHHELPER_H_
#define MANTID_ICAT_SEARCHHELPER_H_

#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

/**  CSearchHelper is a utility class used in Mantid-ICat algorithms to do ICat searching.
     
    @author Sofia Antony, ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
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
		class DLLExport CSearchHelper
		{
		public:
			/// constructor
			CSearchHelper(){}
			/// destructor
			~CSearchHelper(){}
			
			/// search method
			int doSearch(ICATPortBindingProxy& icat,boost::shared_ptr<ns1__searchByAdvanced>& request,ns1__searchByAdvancedResponse& response);
			
			/// method to search by run number
			int doSearchByRunNumber(const double& dstartRun,const double& dendRun,bool bCase,const std::string& instrName,ns1__investigationInclude einclude,
				API::ITableWorkspace_sptr& responsews_sptr);

			/// calls getInvestigationIncludes api's
			int getDataFiles(long long invId,ns1__investigationInclude inclide,API::ITableWorkspace_sptr& responsews_sptr);

			/// this method calls Icat api getInvestigationIncludes and returns datasets for the given investigation id.
			int doDataSetsSearch(long long invId,ns1__investigationInclude inclide,API::ITableWorkspace_sptr& responsews_sptr);

			/// This method lists the isntruments
			API::ITableWorkspace_sptr listInstruments();

			/// This method disconnects last connected  session from icat DB
			int doLogout();
			

		private:
			
			/// This method sets the request parameters for investigation includes.
			void setReqParamforInvestigationIncludes(long long invstId,ns1__investigationInclude include,ns1__getInvestigationIncludes& request);

			/// set request param for investigation includes
			void setReqParamforSearchByRunNumber(const double& dstart,const double& dend,bool bCase,ns1__investigationInclude einclude,
				boost::shared_ptr<ns1__searchByAdvanced>& request);
	
			///This method saves the file search response to table workspace
			API::ITableWorkspace_sptr saveFileSearchResponse(const ns1__searchByAdvancedResponse& response);

			/// This method saves the response data of search by run number to table workspace
			API::ITableWorkspace_sptr saveSearchByRunNumberResponse(const ns1__searchByAdvancedResponse& response);

			/// this method saves investigation include response to a table workspace
			API::ITableWorkspace_sptr saveInvestigationIncludesResponse(const ns1__getInvestigationIncludesResponse& response);

			/// This method saves Datasets to a table workspace
			API::ITableWorkspace_sptr saveDataSets(const ns1__getInvestigationIncludesResponse& response);

			/// This method sets the request parameters
			void setReqparamforlistInstruments(ns1__listInstruments& request);

			/// This method saves Instrument List to a table workspace
			API::ITableWorkspace_sptr saveInstrumentList(const ns1__listInstrumentsResponse& response);

			/// This method creates table workspace
			API::ITableWorkspace_sptr createTableWorkspace();


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
			
		};
	}
}
#endif
