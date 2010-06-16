#include "MantidICat/SearchHelper.h"
#include"MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <iomanip>

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		/* This method calls ICat API searchbydavanced and do the basic search
		 * @param icat Proxy object for ICat
		 * @param request request object
		 * @param response response object
		 */

		int CSearchHelper::doSearch(ICATPortBindingProxy& icat,boost::shared_ptr<ns1__searchByAdvanced>& request,ns1__searchByAdvancedResponse& response)
		{
			// Define ssl authentication scheme
			if (soap_ssl_client_context(&icat,
				SOAP_SSL_NO_AUTHENTICATION, /* use SOAP_SSL_DEFAULT in production code */
				NULL,       /* keyfile: required only when client must authenticate to
							server (see SSL docs on how to obtain this file) */
							NULL,       /* password to read the keyfile */
							NULL,      /* optional cacert file to store trusted certificates */
							NULL,      /* optional capath to directory with trusted certificates */
							NULL      /* if randfile!=NULL: use a file with random data to seed randomness */
							))
			{
				icat.soap_stream_fault(std::cerr);
				return -1;
			}
			//std::cout<<"start run in CSearchHelper do search is "<<*request.advancedSearchDetails->runStart<<std::endl;
			//std::cout<<"end  run in CSearchHelper do search is "<<*request.advancedSearchDetails->runEnd<<std::endl;
            clock_t start=clock();
			int ret_advsearch=icat.searchByAdvanced(request.get(),&response);
			if(ret_advsearch!=0)
			{
				icat.soap_stream_fault(std::cerr);
			}
			clock_t end=clock();
			float diff = float(end -start)/CLOCKS_PER_SEC;
			std::cout<< "Time taken by dosearch is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
			std::cout<<"CSearchHelper do search no of records is "<<response.return_.size()<<std::endl;
			return ret_advsearch;
		}
		int CSearchHelper::dotest(ICATPortBindingProxy& icat,ns1__searchByAdvanced* request,ns1__searchByAdvancedResponse& response)
		{
			if (soap_ssl_client_context(&icat,
				SOAP_SSL_NO_AUTHENTICATION, /* use SOAP_SSL_DEFAULT in production code */
				NULL,       /* keyfile: required only when client must authenticate to
							server (see SSL docs on how to obtain this file) */
							NULL,       /* password to read the keyfile */
							NULL,      /* optional cacert file to store trusted certificates */
							NULL,      /* optional capath to directory with trusted certificates */
							NULL      /* if randfile!=NULL: use a file with random data to seed randomness */
							))
			{
				icat.soap_stream_fault(std::cerr);
				return -1;
			}
			//std::cout<<"start run in CSearchHelper do search is "<<*request.advancedSearchDetails->runStart<<std::endl;
			//std::cout<<"end  run in CSearchHelper do search is "<<*request.advancedSearchDetails->runEnd<<std::endl;
            clock_t start=clock();
			int ret_advsearch=icat.searchByAdvanced(request,&response);
			if(ret_advsearch!=0)
			{
				icat.soap_stream_fault(std::cerr);
			}
			clock_t end=clock();
			float diff = float(end -start)/CLOCKS_PER_SEC;
			std::cout<< "Time taken by dosearch is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
			std::cout<<"CSearchHelper do search no of records is "<<response.return_.size()<<std::endl;
			return ret_advsearch;
		}

		/* This method does a search  by run number and returns investigation data
		 * @param dstartRun start run number
		 * @param dendRun end run number
		 * @param einclude enum to filter the search response
		 * @param responsews_sptr output table workspace
		 */

		int CSearchHelper::doSearchByRunNumber(const double& dstartRun,const double& dendRun,ns1__investigationInclude einclude,
			               API::ITableWorkspace_sptr& responsews_sptr)
		{
			//ICAt proxy object
			ICATPortBindingProxy icat;
			// request object
			//ns1__searchByAdvanced request;
			boost::shared_ptr<ns1__searchByAdvanced> req_sptr(new ns1__searchByAdvanced );
			boost::shared_ptr<std::string > sessionId_sptr(new std::string);
			req_sptr->sessionId=sessionId_sptr.get();
			boost::shared_ptr<ns1__advancedSearchDetails>adv_sptr(new ns1__advancedSearchDetails);
			req_sptr->advancedSearchDetails=adv_sptr.get();
			//run start
			//boost::shared_ptr<double>runstart_sptr(new double);
		//	req_sptr->advancedSearchDetails->runStart=runstart_sptr.get();
			//run end
			//boost::shared_ptr<double> runend_sptr(new double);
			//req_sptr->advancedSearchDetails->runEnd=runend_sptr.get();

			boost::shared_ptr<std::string> dataFilename_sptr(new std::string);
			req_sptr->advancedSearchDetails->datafileName=dataFilename_sptr.get();


			// investigation include
            boost::shared_ptr<ns1__investigationInclude>invstInculde_sptr(new ns1__investigationInclude);
			req_sptr->advancedSearchDetails->investigationInclude=invstInculde_sptr.get();

			//setting the input parameters
			//setRequestParameters(dstartRun,dendRun,einclude,request);
			setReqParamforSearchByRunNumber(dstartRun,dendRun,einclude,req_sptr);

			//response object
			ns1__searchByAdvancedResponse response;
			// do  search
			int ret_search=doSearch(icat,req_sptr,response);
			if(ret_search==0)
			{
				///save response to a table workspace
				responsews_sptr=saveResponseDatatoTableWorkspace(einclude,response) ;
			}
			else
			{
				//replace with mantid error routine
				icat.soap_stream_fault(std::cerr);
			}
			return ret_search;
		}
		/* This method sets the input request parameters for search
		* @param dstart start run number
		* @param dend end run number
		* @param einclude enum paramter to specify the response records
		* @request input request object
		*/
		void CSearchHelper::setReqParamforSearchByRunNumber(const double& dstart,const double& dend,ns1__investigationInclude einclude,
			                boost::shared_ptr<ns1__searchByAdvanced>& request)
		{
			//get the sessionid which is cached in session class during login
			*request->sessionId=Session::Instance().getSessionId();
			request->advancedSearchDetails->caseSensitive=false;
			//run start
		    //*request->advancedSearchDetails->runStart=dstart;
			//*request->advancedSearchDetails->runEnd=dend;
			*request->advancedSearchDetails->datafileName="GEM1910*";
			*request->advancedSearchDetails->investigationInclude=einclude;
			}

		/* This method sets the input request parameters for search
		 * @param dstart start run number
		 * @param dend end run number
		 * @param einclude enum paramter to specify the response records
		 * @request input request object
		 */

		void CSearchHelper::setRequestParameters(const double& dstart,const double& dend,ns1__investigationInclude einclude,
			                ns1__searchByAdvanced& request)
		{

			//get the sessionid which is cached in session class during login
			m_sessionId_sptr=boost::shared_ptr<std::string >(new std::string);
			*m_sessionId_sptr=Session::Instance().getSessionId();
			request.sessionId=m_sessionId_sptr.get();

			m_advanceDetails_sptr=boost::shared_ptr<ns1__advancedSearchDetails>(new ns1__advancedSearchDetails);
			m_advanceDetails_sptr->caseSensitive=false;

			//run start
			m_runStart_sptr =boost::shared_ptr<double> (new double);
			*m_runStart_sptr=dstart;
			m_advanceDetails_sptr->runStart=m_runStart_sptr.get();

			//run end
			m_runEnd_sptr=boost::shared_ptr<double>(new double);
			*m_runEnd_sptr=dend;
			m_advanceDetails_sptr->runEnd=m_runEnd_sptr.get();

			// enum include
			m_invstInculde_sptr=boost::shared_ptr<ns1__investigationInclude>(new ns1__investigationInclude);
			*m_invstInculde_sptr=einclude;

			m_advanceDetails_sptr->investigationInclude=m_invstInculde_sptr.get();
			request.advancedSearchDetails=m_advanceDetails_sptr.get();

		}
		/* This method saves the search response data to a table workspace
		 * @param einclude enum to identfy the search result
		 *  @param response const reference to response object
		 *  @returns shared pointer to table workspace which stores the data
		 */
		API::ITableWorkspace_sptr CSearchHelper::saveResponseDatatoTableWorkspace(ns1__investigationInclude einclude,
			                                     const ns1__searchByAdvancedResponse& response)
		{
			API::ITableWorkspace_sptr outputws ;
			if(einclude==ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES)
			{
				outputws=saveSearchByRunNumberResponse(response);
			}
			/*else if(einclude==ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES)
			{
				outputws=saveFileSearchResponse(response);
			}*/
			return outputws;
		}

		/* This method loops through the return_ vector and saves the investigation data to a table workspace
		*  @param response const reference to response object
		*  @returns shared pointer to table workspace which stores the data
		*/
		API::ITableWorkspace_sptr CSearchHelper::saveSearchByRunNumberResponse(const ns1__searchByAdvancedResponse& response)
		{
			//create table workspace
			API::ITableWorkspace_sptr outputws ;
			try
			{
				outputws=WorkspaceFactory::Instance().createTable("TableWorkspace");
			}
			catch(Mantid::Kernel::Exception::NotFoundError& )
			{
				throw;
			}
			outputws->addColumn("long64","InvestigationId");
			outputws->addColumn("str","RbNumber");
			outputws->addColumn("str","Title");
			outputws->addColumn("str","Type");
			outputws->addColumn("str","Instrument");
			outputws->addColumn("str","Investigator");
			outputws->addColumn("str","RunRange");
			outputws->addColumn("str","Year");
			try
			{
				std::vector<ns1__investigation*>::const_iterator citr;
				for (citr=response.return_.begin();citr!=response.return_.end();++citr)
				{
					API::TableRow t = outputws->appendRow();

					//investigation id
					savetoTableWorkspace((*citr)->id,t);
					//rb number
					savetoTableWorkspace((*citr)->invNumber,t);

					//std::cout<<"search by run number: rb number is : "<<*(*citr)->invNumber<<std::endl;

					//title
					savetoTableWorkspace((*citr)->title,t);

					//std::cout<<"search by runnumber: title is : "<<*(*citr)->title<<std::endl;

					//type
					savetoTableWorkspace((*citr)->invType,t);
					savetoTableWorkspace((*citr)->instrument,t);
					//investigator
					savetoTableWorkspace((*citr)->bcatInvStr,t);
					// run range
					savetoTableWorkspace((*citr)->invParamValue,t);
					//year
					if((*citr)->invEndDate!=NULL)
					{
						time_t  invEndtime=*(*citr)->invEndDate;
						char temp [25];
						strftime (temp,25,"%H:%M:%S %Y-%d-%b",localtime(&invEndtime));
						std::string ftime(temp);
						std::string *sInvEndtime=new std::string ;
						sInvEndtime->assign(ftime);
						savetoTableWorkspace(sInvEndtime,t);
					}
				}
			}
			catch(std::runtime_error& )
			{
			  throw;
			}

			return outputws;
		}
		/* This method loops through the response return_vector and saves the datafile details to a table workspace
		 * @param response const reference to response object
		 * @returns shared pointer to table workspace which stores the data
		*/
		API::ITableWorkspace_sptr CSearchHelper::saveFileSearchResponse(const ns1__searchByAdvancedResponse& response)
		{
			//create table workspace
			API::ITableWorkspace_sptr outputws ;
			try
			{
				outputws=WorkspaceFactory::Instance().createTable("TableWorkspace");
			}
			catch(Mantid::Kernel::Exception::NotFoundError& )
			{
				throw;
			}
			outputws->addColumn("str","Name");
			outputws->addColumn("int","File Size");
			outputws->addColumn("long64","FileId");
			outputws->addColumn("str","Format");
			outputws->addColumn("str","Format Version");
			outputws->addColumn("str","Format Type");
			outputws->addColumn("str","Create Time");

			std::vector<ns1__investigation*> investVec;
			investVec.assign(response.return_.begin(),response.return_.end());

			try
			{
				std::vector<ns1__investigation*>::const_iterator inv_citr;
				for (inv_citr=investVec.begin();inv_citr!=investVec.end();++inv_citr)
				{
					std::vector<ns1__dataset*> datasetVec;
					datasetVec.assign((*inv_citr)->datasetCollection.begin(),(*inv_citr)->datasetCollection.end());

					std::vector<ns1__dataset*>::const_iterator dataset_citr;
					for(dataset_citr=datasetVec.begin();dataset_citr!=datasetVec.end();++dataset_citr)
					{
						std::vector<ns1__datafile * >datafileVec;
						datafileVec.assign((*dataset_citr)->datafileCollection.begin(),(*dataset_citr)->datafileCollection.end());

						std::vector<ns1__datafile * >::const_iterator datafile_citr;
						for(datafile_citr=datafileVec.begin();datafile_citr!=datafileVec.end();++datafile_citr)
						{

							API::TableRow t = outputws->appendRow();

							std::string fileName=*(*datafile_citr)->name;
							//std::cout<<"File name is "<<fileName<<std::endl;
							savetoTableWorkspace((*datafile_citr)->name,t);

							int fileSize=*(*datafile_citr)->fileSize;
							//std::cout<<"File size is "<<fileSize<<std::endl;
							savetoTableWorkspace((*datafile_citr)->fileSize,t);

							long long fileId=*(*datafile_citr)->id;
							//std::cout<<"File id is "<<fileId<<std::endl;
							savetoTableWorkspace((*datafile_citr)->id,t);
							ns1__datafileFormat* fileFormat=(*datafile_citr)->datafileFormat;
							if(fileFormat)
							{
								if(fileFormat->datafileFormatPK)
								{
									std::string format=*(fileFormat->datafileFormatPK->name);
									//std::cout<<"File format is  "<<format<<std::endl;
									savetoTableWorkspace((fileFormat->datafileFormatPK->name),t);

									std::string version=*(fileFormat->datafileFormatPK->version);
									//std::cout<<"File format version  is "<<version<<std::endl;
									savetoTableWorkspace((fileFormat->datafileFormatPK->version),t);
								}
								std::string formatType=*(fileFormat->formatType);
								//std::cout<<"File format type  is "<<formatType<<std::endl;
								savetoTableWorkspace((fileFormat->formatType),t);

							}
							if((*datafile_citr)->datafileCreateTime!=NULL)
							{
								time_t  crtime=*(*datafile_citr)->datafileCreateTime;
								char temp [25];
								strftime (temp,25,"%H:%M:%S %Y-%d-%b",localtime(&crtime));
								std::string ftime(temp);
								std::string *creationtime=new std::string ;
								creationtime->assign(ftime);
								savetoTableWorkspace(creationtime,t);
							}
							//std::cout<<"File creation time  is "<<*creationtime<<std::endl;



						}

					}


				}
			}

			catch(std::runtime_error& )
			{
				throw;
			}

			return outputws;
		}
		void CSearchHelper::setReqParamforInvestigationIncludes(long long invstId,ns1__investigationInclude include,ns1__getInvestigationIncludes& request)
		{
			//get the sessionid which is cached in session class during login
			// m_sessionId_sptr=boost::shared_ptr<std::string >(new std::string);
			//*m_sessionId_sptr=Session::Instance().getSessionId();
			*request.sessionId=Session::Instance().getSessionId();;

			// enum include
			//*m_invstInculde_sptr=include;
			*request.investigationInclude=include;

			// m_invstId=boost::shared_ptr<long long>(new long long);
		     //*m_invstId=invstId;
			 *request.investigationId=invstId;


		}
		int CSearchHelper::dogetInvestigationIncludes(long long invstId,ns1__investigationInclude include,
			               API::ITableWorkspace_sptr& responsews_sptr)
		{
			//ICAt proxy object
			ICATPortBindingProxy icat;

			// Define ssl authentication scheme
			if (soap_ssl_client_context(&icat,
				SOAP_SSL_NO_AUTHENTICATION, /* use SOAP_SSL_DEFAULT in production code */
				NULL,       /* keyfile: required only when client must authenticate to
							server (see SSL docs on how to obtain this file) */
							NULL,       /* password to read the keyfile */
							NULL,      /* optional cacert file to store trusted certificates */
							NULL,      /* optional capath to directory with trusted certificates */
							NULL      /* if randfile!=NULL: use a file with random data to seed randomness */
							))
			{
				icat.soap_stream_fault(std::cerr);
				return -1;
			}

			ns1__getInvestigationIncludes request;
			//get the sessionid which is cached in session class during login
			boost::shared_ptr<std::string >sessionId_sptr(new std::string);
			request.sessionId=sessionId_sptr.get();

			// enum include
			boost::shared_ptr<ns1__investigationInclude>invstInculde_sptr(new ns1__investigationInclude);
			request.investigationInclude=invstInculde_sptr.get();

			//boost::shared_ptr<LONG64>invstId_sptr(new LONG64);
		  //  request.investigationId=invstId_sptr.get();
			setReqParamforInvestigationIncludes(invstId,include,request);

			ns1__getInvestigationIncludesResponse response;
			int ret_advsearch=icat.getInvestigationIncludes(&request,&response);
			if(ret_advsearch!=0)
			{
				icat.soap_stream_fault(std::cerr);
				return 0;// handle error
			}
			responsews_sptr=saveInvestigationIncludesResponse(response);

			return ret_advsearch;
		}
		/* This method loops through the response return_vector and saves the datafile details to a table workspace
		 * @param response const reference to response object
		 * @returns shared pointer to table workspace which stores the data
		*/
		API::ITableWorkspace_sptr CSearchHelper::saveInvestigationIncludesResponse(const ns1__getInvestigationIncludesResponse& response)
		{
			//create table workspace
			API::ITableWorkspace_sptr outputws ;
			try
			{
				outputws=WorkspaceFactory::Instance().createTable("TableWorkspace");
			}
			catch(Mantid::Kernel::Exception::NotFoundError& )
			{
				throw;
			}
			outputws->addColumn("str","Instrument");//Instrument name
			outputws->addColumn("str","Name");//File name
			outputws->addColumn("int","File Size");
			outputws->addColumn("long64","FileId");
			outputws->addColumn("str","Format");
			outputws->addColumn("str","Format Version");
			outputws->addColumn("str","Format Type");
			outputws->addColumn("str","Create Time");
			try
			{

					std::vector<ns1__dataset*> datasetVec;
					datasetVec.assign((response.return_)->datasetCollection.begin(),(response.return_)->datasetCollection.end());

					std::vector<ns1__dataset*>::const_iterator dataset_citr;
					for(dataset_citr=datasetVec.begin();dataset_citr!=datasetVec.end();++dataset_citr)
					{
						std::vector<ns1__datafile * >datafileVec;
						datafileVec.assign((*dataset_citr)->datafileCollection.begin(),(*dataset_citr)->datafileCollection.end());

						std::vector<ns1__datafile * >::const_iterator datafile_citr;
						for(datafile_citr=datafileVec.begin();datafile_citr!=datafileVec.end();++datafile_citr)
						{

							API::TableRow t = outputws->appendRow();

							std::string instrName=*response.return_->instrument;
							std::cout<<"Instrument name is "<<instrName<<std::endl;
							savetoTableWorkspace(response.return_->instrument,t);

							std::string fileName=*(*datafile_citr)->name;
							std::cout<<"File name is "<<fileName<<std::endl;
							savetoTableWorkspace((*datafile_citr)->name,t);

							int fileSize=*(*datafile_citr)->fileSize;
							std::cout<<"File size is "<<fileSize<<std::endl;
							savetoTableWorkspace((*datafile_citr)->fileSize,t);

							long long fileId=*(*datafile_citr)->id;
							std::cout<<"File id is "<<fileId<<std::endl;
							savetoTableWorkspace((*datafile_citr)->id,t);
							ns1__datafileFormat* fileFormat=(*datafile_citr)->datafileFormat;
							if(fileFormat)
							{
								if(fileFormat->datafileFormatPK)
								{
									std::string format=*(fileFormat->datafileFormatPK->name);
									std::cout<<"File format is  "<<format<<std::endl;
									savetoTableWorkspace((fileFormat->datafileFormatPK->name),t);

									std::string version=*(fileFormat->datafileFormatPK->version);
									std::cout<<"File format version  is "<<version<<std::endl;
									savetoTableWorkspace((fileFormat->datafileFormatPK->version),t);
								}
								std::string formatType=*(fileFormat->formatType);
								std::cout<<"File format type  is "<<formatType<<std::endl;
								savetoTableWorkspace((fileFormat->formatType),t);

							}
							if((*datafile_citr)->datafileCreateTime!=NULL)
							{
								time_t  crtime=*(*datafile_citr)->datafileCreateTime;

								char temp [25];
								strftime (temp,25,"%H:%M:%S %Y-%d-%b",localtime(&crtime));
								std::string ftime(temp);
								std::string *creationtime=new std::string ;
								creationtime->assign(ftime);

								savetoTableWorkspace(creationtime,t);
								std::cout<<"File creation time  is "<<*creationtime<<std::endl;
							}

						}

					}

			}

			catch(std::runtime_error& )
			{
				throw;
			}

			return outputws;
		}

	}
}
