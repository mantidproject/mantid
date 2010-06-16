#ifndef MANTID_ICAT_SEARCHHELPER_H
#define MANTID_ICAT_SEARCHHELPER_H

#include "MantidAPI/Algorithm.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
namespace Mantid
{
	namespace ICat
	{
		class DLLExport CSearchHelper
		{
		public:
			CSearchHelper(){}
			~CSearchHelper(){}
			
			/// search method
			int doSearch(ICATPortBindingProxy& icat,boost::shared_ptr<ns1__searchByAdvanced>& request,ns1__searchByAdvancedResponse& response);
			int dotest(ICATPortBindingProxy& icat,ns1__searchByAdvanced* request,ns1__searchByAdvancedResponse& response);
			///
			int doSearchByRunNumber(const double& dstartRun,const double& dendRun,ns1__investigationInclude einclude,
				API::ITableWorkspace_sptr& responsews_sptr);

			///
			int dogetInvestigationIncludes(long long invId,ns1__investigationInclude inclide,API::ITableWorkspace_sptr& responsews_sptr);
		private:
		
			/// this method sets the request parametrs for serch by runnumber
			void setRequestParameters(const double& dstart,const double& dend,ns1__investigationInclude einclude,
				 ns1__searchByAdvanced& request);

			/// this method saves the response data to table workspace.
			API::ITableWorkspace_sptr saveResponseDatatoTableWorkspace(ns1__investigationInclude einclude,const ns1__searchByAdvancedResponse& response);

			///
			API::ITableWorkspace_sptr saveFileSearchResponse(const ns1__searchByAdvancedResponse& response);

			///
			API::ITableWorkspace_sptr saveSearchByRunNumberResponse(const ns1__searchByAdvancedResponse& response);

			///
			void setReqParamforInvestigationIncludes(long long invstId,ns1__investigationInclude include,ns1__getInvestigationIncludes& request);
			///
			void setReqParamforSearchByRunNumber(const double& dstart,const double& dend,ns1__investigationInclude einclude,
			                boost::shared_ptr<ns1__searchByAdvanced>& request);

			///
			API::ITableWorkspace_sptr saveInvestigationIncludesResponse(const ns1__getInvestigationIncludesResponse& response);


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
			boost::shared_ptr<std::string> m_sessionId_sptr;
			boost::shared_ptr<std::string> m_visitId_sptr;
			boost::shared_ptr< ns1__investigationInclude> m_invstInculde_sptr;
            boost::shared_ptr<double>m_runStart_sptr;
			boost::shared_ptr<double>m_runEnd_sptr;
			boost::shared_ptr<ns1__advancedSearchDetails>m_advanceDetails_sptr;
			boost::shared_ptr<long long>m_invstId;
		};
	}
}
#endif