#include "MantidICat/SearchByRunNumber.h"
#include"MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include"MantidICat/SearchHelper.h"
#include <iomanip>
namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CSearchByRunNumber)
		/// Initialisation method.
		void CSearchByRunNumber::init()
		{
			BoundedValidator<double>* mustBePositive = new BoundedValidator<double>();
			mustBePositive->setLower(0.0);
			declareProperty("StartRun",0.0,mustBePositive,"The start run number");
			declareProperty("EndRun",0.0,mustBePositive->clone(),"The end run number");
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
				"The name of the workspace that will be created to store the search result");
		}
		/// Execution method.
		void CSearchByRunNumber::exec()
		{
			
			API::ITableWorkspace_sptr ws_sptr=doSearchByRunNumber();
			setProperty("OutputWorkspace",ws_sptr);
		}

		/* This method does search by run number.
		 * @returns shared pointer to table workspace which stores the data
		*/
		API::ITableWorkspace_sptr  CSearchByRunNumber::doSearchByRunNumber()
		{	
			double dstartRun=getProperty("StartRun");
			double dendRun=getProperty("EndRun");

			API::ITableWorkspace_sptr outputws;
			CSearchHelper searchobj;
			//int ret_advsearch=searchobj.doSearchByRunNumber(dstartRun,dendRun,ns1__investigationInclude__INVESTIGATORS_USCOREONLY,outputws);
			int ret_advsearch=searchobj.doSearchByRunNumber(dstartRun,dendRun,ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,outputws);
			
			//need to handle error properly once i get error details from Tom
			if(ret_advsearch!=0)
			{
				throw;
			}
			return outputws;
		
		//pagination 
		//	//ICAt proxy object
		//	ICATPortBindingProxy icat;
		//	// Define ssl authentication scheme
		//	if (soap_ssl_client_context(&icat,
		//		SOAP_SSL_NO_AUTHENTICATION, /* use SOAP_SSL_DEFAULT in production code */
		//		NULL,       /* keyfile: required only when client must authenticate to 
		//					server (see SSL docs on how to obtain this file) */
		//					NULL,       /* password to read the keyfile */
		//					NULL,      /* optional cacert file to store trusted certificates */
		//					NULL,      /* optional capath to directory with trusted certificates */
		//					NULL      /* if randfile!=NULL: use a file with random data to seed randomness */ 
		//					))
		//	{ 
		//		icat.soap_stream_fault(std::cerr);
		//		//return -1;
		//	}
		//std::string sessionId=Session::Instance().getSessionId();
		//ns1__searchByAdvancedPagination request;
		//request.sessionId=&sessionId;
		//request.startIndex=0;
		//request.numberOfResults=200;

		//request.advancedSearchDetails=new ns1__advancedSearchDetails;
		// 
		//request.advancedSearchDetails->runStart=&dstartRun;
		//request.advancedSearchDetails->runEnd=&dendRun;
		//int einclude=ns1__investigationInclude__INVESTIGATORS_USCOREONLY;	
		//request.advancedSearchDetails->investigationInclude=(ns1__investigationInclude*)&einclude;

		//ns1__searchByAdvancedPaginationResponse response;

		//clock_t start=clock();
		//int ret_advsearch=icat.searchByAdvancedPagination(&request,&response);
		//if(ret_advsearch!=0)
		//{
		//	std::cout<<"Error in pagination api"<<std::endl;
		//	throw;
		//}
		//clock_t end=clock();
		//float diff = float(end -start)/CLOCKS_PER_SEC;
		//std::cout<< "Time taken by dosearch is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
		//std::cout<<"no of records is "<<response.return_.size();
			

		}

	
	}
}

