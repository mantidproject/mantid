// WorkspaceFactory include must be first otherwise you get a bizarre Poco-related compilation error on Windows
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidICat/SearchHelper.h"
#include "MantidICat/Session.h"
#include "MantidICat/ErrorHandling.h" 
#include <iomanip>

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		/* This method calls ICat API searchbydavanced and do the basic run search 
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
				//icat.soap_stream_fault(std::cerr);
				CErrorHandling::throwErrorMessages(icat);
				
			}
			clock_t start=clock();
			int ret_advsearch=icat.searchByAdvanced(request.get(),&response);
			if(ret_advsearch!=0)
			{
				//icat.soap_stream_fault(std::cerr);
				CErrorHandling::throwErrorMessages(icat);
			}
			clock_t end=clock();
			float diff = float(end -start)/CLOCKS_PER_SEC;
			//g_log.information()<<" Time taken to do  search by run number and instrument is "<<diff<<std::endl;
			//std::cout<< "Time taken by dosearch is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
			//std::cout<<"CSearchHelper do search no of records is "<<response.return_.size()<<std::endl;
			return ret_advsearch;
		}

		/* This method does a search  by run number and returns investigation data
		 * @param dstartRun start run number
		 * @param dendRun end run number
		 * @param instrument name of the instrument
		 * @param einclude enum to filter the search response
		 * @param responsews_sptr output table workspace
		 */
		int CSearchHelper::doSearchByRunNumber(const double& dstartRun,const double& dendRun,bool bCase,const std::string& instrument,
			               ns1__investigationInclude einclude,API::ITableWorkspace_sptr& responsews_sptr)
		{
			//ICAt proxy object
			ICATPortBindingProxy icat;
			// request object
			boost::shared_ptr<ns1__searchByAdvanced> req_sptr(new ns1__searchByAdvanced );
			boost::shared_ptr<std::string > sessionId_sptr(new std::string);
			req_sptr->sessionId=sessionId_sptr.get();
			boost::shared_ptr<ns1__advancedSearchDetails>adv_sptr(new ns1__advancedSearchDetails);
			req_sptr->advancedSearchDetails=adv_sptr.get();
			//run start
			boost::shared_ptr<double>runstart_sptr(new double);
			req_sptr->advancedSearchDetails->runStart=runstart_sptr.get();
			//run end
			boost::shared_ptr<double>runend_sptr(new double);
			req_sptr->advancedSearchDetails->runEnd=runend_sptr.get();
			
			//instrument name
			req_sptr->advancedSearchDetails->instruments.push_back(instrument);

			// investigation include
            boost::shared_ptr<ns1__investigationInclude>invstInculde_sptr(new ns1__investigationInclude);
			req_sptr->advancedSearchDetails->investigationInclude=invstInculde_sptr.get();

			//setting the input parameters
			setReqParamforSearchByRunNumber(dstartRun,dendRun,bCase,einclude,req_sptr);

			//response object
			ns1__searchByAdvancedResponse response;
			// do  search
			int ret_search=doSearch(icat,req_sptr,response);
			if(ret_search!=0)
			{
				//replace with mantid error routine
				//icat.soap_stream_fault(std::cerr);
				CErrorHandling::throwErrorMessages(icat);
				
			}
			if(response.return_.empty())
			{
				throw std::runtime_error("ICat investigations search is complete.There are no results to display");
			}
			//save response to a table workspace
			responsews_sptr=saveSearchByRunNumberResponse(response);
			return ret_search;
		}
	   /* This method sets the input request parameters for search 
		* @param dstart start run number
		* @param dend end run number
		* @param einclude enum paramter to specify the response records
		* @request refrence to request object
		*/
		void CSearchHelper::setReqParamforSearchByRunNumber(const double& dstart,const double& dend,bool bCase,ns1__investigationInclude einclude,
			                boost::shared_ptr<ns1__searchByAdvanced>& request)
		{
			//get the sessionid which is cached in session class during login
			*request->sessionId=Session::Instance().getSessionId();
			request->advancedSearchDetails->caseSensitive=bCase;
			//run start
		    *request->advancedSearchDetails->runStart=dstart;
			//run end
			*request->advancedSearchDetails->runEnd=dend;
			// investigation include
			*request->advancedSearchDetails->investigationInclude=einclude;
			}

		
	   /* This method saves the search response( investigations )data to a table workspace
		*  @param response const reference to response object
		*  @returns shared pointer to table workspace which stores the data
		*/
		API::ITableWorkspace_sptr CSearchHelper::saveSearchByRunNumberResponse(const ns1__searchByAdvancedResponse& response)
		{
			//create table workspace
		
			API::ITableWorkspace_sptr outputws =createTableWorkspace();

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
					//title
					savetoTableWorkspace((*citr)->title,t);
                   				
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
			  throw std::runtime_error("Error when saving  the ICat Search Results data to Workspace");
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
			API::ITableWorkspace_sptr outputws =createTableWorkspace();
			//add columns
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
							savetoTableWorkspace((*datafile_citr)->name,t);
							savetoTableWorkspace((*datafile_citr)->fileSize,t);

							//long long fileId=*(*datafile_citr)->id;
							//std::cout<<"File id is "<<fileId<<std::endl;
							savetoTableWorkspace((*datafile_citr)->id,t);
							ns1__datafileFormat* fileFormat=(*datafile_citr)->datafileFormat;
							if(fileFormat)
							{
								if(fileFormat->datafileFormatPK)
								{
									//std::string format=*(fileFormat->datafileFormatPK->name);
									//std::cout<<"File format is  "<<format<<std::endl;
									savetoTableWorkspace((fileFormat->datafileFormatPK->name),t);

									//std::string version=*(fileFormat->datafileFormatPK->version);
									//std::cout<<"File format version  is "<<version<<std::endl;
									savetoTableWorkspace((fileFormat->datafileFormatPK->version),t);
								}
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
		/* This method sets the request parameters for the investigations includes
		 * @param invstId - investigation id 
		 * @param include - enum parameter to retrieve dat from DB
		 * @param request - request object
		*/
		void CSearchHelper::setReqParamforInvestigationIncludes(long long invstId,ns1__investigationInclude include,ns1__getInvestigationIncludes& request)
		{
			//get the sessionid which is cached in session class during login
			*request.sessionId=Session::Instance().getSessionId();;
			*request.investigationInclude=include;
  		    *request.investigationId=invstId;

		}
		/**This method calls ICat API getInvestigationIncludes and returns investigation details for a given investigation Id
		  *@param invstId - investigation id
		  *@param include - enum parameter for selecting the response data from the db.
		  *@param responsews_sptr - table workspace to save the response data
		*/
		int CSearchHelper::getDataFiles(long long invstId,ns1__investigationInclude include,
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
				//icat.soap_stream_fault(std::cerr);
				CErrorHandling::throwErrorMessages(icat);
			}

			ns1__getInvestigationIncludes request;
			//get the sessionid which is cached in session class during login
			boost::shared_ptr<std::string >sessionId_sptr(new std::string);
			request.sessionId=sessionId_sptr.get();

			// enum include
			boost::shared_ptr<ns1__investigationInclude>invstInculde_sptr(new ns1__investigationInclude);
			request.investigationInclude=invstInculde_sptr.get();

			//boost::shared_ptr<long long>invstId_sptr(new long long);
		   // request.investigationId=invstId_sptr.get();
			request.investigationId=new LONG64;
			setReqParamforInvestigationIncludes(invstId,include,request);

			ns1__getInvestigationIncludesResponse response;
			int ret_advsearch=icat.getInvestigationIncludes(&request,&response);
			if(ret_advsearch!=0)
			{
				CErrorHandling::throwErrorMessages(icat);
			}
			std::stringstream stream;
			stream<<invstId;
			if(!response.return_)
			{
				throw std::runtime_error("No data files exists in the database for the selected investigation");
			}
			try
			{
				responsews_sptr=saveInvestigationIncludesResponse(response);
			}
			catch(std::runtime_error)
			{
				
				throw std::runtime_error("Error when selecting the investigation data with inestigation id "+ stream.str());
			}
			return ret_advsearch;
		}
		/* This method loops through the response return_vector and saves the datafile details to a table workspace
		 * @param response const reference to response object
		 * @returns shared pointer to table workspace which stores the data
		*/
		API::ITableWorkspace_sptr CSearchHelper::saveInvestigationIncludesResponse(const ns1__getInvestigationIncludesResponse& response)
		{
			//create table workspace
			API::ITableWorkspace_sptr outputws =createTableWorkspace();

			//outputws->addColumn("str","Instrument");//Instrument name
			//outputws->addColumn("long64","InvestigationId");//investigation id
			outputws->addColumn("str","Name");//File name
			outputws->addColumn("int","File Size");//File Size
			outputws->addColumn("long64","File Id");//File id
			outputws->addColumn("str","Format");//File Format
			outputws->addColumn("str","Format Version");//File Version
			outputws->addColumn("str","Format Type");// File Format Type
			outputws->addColumn("str","Create Time");// File Creation Time

			try
			{		std::vector<ns1__dataset*> datasetVec;
					datasetVec.assign((response.return_)->datasetCollection.begin(),(response.return_)->datasetCollection.end());
					if(datasetVec.empty())
					{
						throw std::runtime_error("No data files exists in the database for the selected investigation");
					}


					std::vector<ns1__dataset*>::const_iterator dataset_citr;
					for(dataset_citr=datasetVec.begin();dataset_citr!=datasetVec.end();++dataset_citr)
					{
						std::vector<ns1__datafile * >datafileVec;
						datafileVec.assign((*dataset_citr)->datafileCollection.begin(),(*dataset_citr)->datafileCollection.end());
						if(datafileVec.empty())
						{
							throw std::runtime_error("No data files exists in the database for the selected  investigation ");
						}

						std::vector<ns1__datafile * >::const_iterator datafile_citr;
						for(datafile_citr=datafileVec.begin();datafile_citr!=datafileVec.end();++datafile_citr)
						{

							API::TableRow t = outputws->appendRow();
							
							//instrument name
							//savetoTableWorkspace(response.return_->instrument,t);
							//investigation Id
							//savetoTableWorkspace(response.return_->id,t);
							// File Name
							savetoTableWorkspace((*datafile_citr)->name,t);
							// File Size
							savetoTableWorkspace((*datafile_citr)->fileSize,t);
							//File Id
							savetoTableWorkspace((*datafile_citr)->id,t);
							ns1__datafileFormat* fileFormat=(*datafile_citr)->datafileFormat;
							if(fileFormat)
							{
								if(fileFormat->datafileFormatPK)
								{
									// File Format
									savetoTableWorkspace((fileFormat->datafileFormatPK->name),t);
									// File Format Version
									savetoTableWorkspace((fileFormat->datafileFormatPK->version),t);
								}
								// File format Type
								savetoTableWorkspace((fileFormat->formatType),t);
							}
							else
							{
								//i've to see a better way to write empty columns if the data is empty
								std::string *s=NULL;
								savetoTableWorkspace(s,t);
								savetoTableWorkspace(s,t);
								savetoTableWorkspace(s,t);
							}
														
							//File creation Time.
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

						}

					}

			}
			catch(std::runtime_error& )
			{
				throw ;
			}

			return outputws;
		}

		/* *This method calls ICat API getInvestigationIncludes and returns datasets details for a given investigation Id
		   *@param invstId - investigation id
		   *@param include - enum parameter for selecting the response data from iact db.
		   *@param responsews_sptr - table workspace to save the response data
		*/
		int CSearchHelper::doDataSetsSearch(long long invstId,ns1__investigationInclude include,
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
				CErrorHandling::throwErrorMessages(icat);
			}

			// request object
			ns1__getInvestigationIncludes request;
			//get the sessionid which is cached in session class during login
			boost::shared_ptr<std::string >sessionId_sptr(new std::string);
			request.sessionId=sessionId_sptr.get();

			// enum include 
			boost::shared_ptr<ns1__investigationInclude>invstInculde_sptr(new ns1__investigationInclude);
			request.investigationInclude=invstInculde_sptr.get();

			request.investigationId=new LONG64;
			setReqParamforInvestigationIncludes(invstId,include,request);
            
			//response object
			ns1__getInvestigationIncludesResponse response;
			// Calling Icat api 
			int ret_advsearch=icat.getInvestigationIncludes(&request,&response);
			if(ret_advsearch!=0)
			{
				CErrorHandling::throwErrorMessages(icat);
			}
			std::stringstream stream;
			stream<<invstId;
			if(!(response.return_)|| (response.return_)->datasetCollection.empty())
			{
				throw std::runtime_error("No datasets  exists in the ICat database for the inestigation id "+ stream.str());
			}
			try
			{
				responsews_sptr=saveDataSets(response);
			}
			catch(std::runtime_error)
			{
				
				throw std::runtime_error("Error when loading the datasets for the inestigation id "+ stream.str());
			}

			return ret_advsearch;
		}

		/* This method loops through the response return_vector and saves the datasets details to a table workspace
		 * @param response const reference to response object
		 * @returns shared pointer to table workspace which stores the data
		*/
		API::ITableWorkspace_sptr CSearchHelper::saveDataSets(const ns1__getInvestigationIncludesResponse& response)
		{
			//create table workspace
			API::ITableWorkspace_sptr outputws =createTableWorkspace();
			//adding columns
			outputws->addColumn("str","Name");//File name
			outputws->addColumn("str","Status");
			outputws->addColumn("str","Type");
			outputws->addColumn("str","Description");
			outputws->addColumn("long64","Sample");
			
			try
			{	
				
				std::vector<ns1__dataset*> datasetVec;
				datasetVec.assign((response.return_)->datasetCollection.begin(),(response.return_)->datasetCollection.end());

				std::vector<ns1__dataset*>::const_iterator dataset_citr;
				for(dataset_citr=datasetVec.begin();dataset_citr!=datasetVec.end();++dataset_citr)
				{						
					API::TableRow t = outputws->appendRow();

					// DataSet Name
					savetoTableWorkspace((*dataset_citr)->name,t);
					// DataSet Status
					savetoTableWorkspace((*dataset_citr)->datasetStatus,t);
					//DataSet Type
					savetoTableWorkspace((*dataset_citr)->datasetType,t);
					//DataSet Type
					savetoTableWorkspace((*dataset_citr)->description,t);
					//DataSet Type
					savetoTableWorkspace((*dataset_citr)->sampleId,t);
				}
			}

			catch(std::runtime_error& )
			{
				throw;
			}

			return outputws;
		}

		/* *This method calls ICat api listruments and returns the list of instruments a table workspace
		   *@return API::ITableWorkspace_sptr
		*/
		API::ITableWorkspace_sptr CSearchHelper::listInstruments()
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
				CErrorHandling::throwErrorMessages(icat);
			}

			ns1__listInstruments request;

			//get the sessionid which is cached in session class during login
			boost::shared_ptr<std::string >sessionId_sptr(new std::string);
			request.sessionId=sessionId_sptr.get();
			//setting the request parameters
			setReqparamforlistInstruments(request);

			// response object
			 ns1__listInstrumentsResponse response;

			 int ret=icat.listInstruments(&request,&response);
			 if(ret!=0)
			 {
				 CErrorHandling::throwErrorMessages(icat);
			 }
			 if(response.return_.empty())
			 {
				 std::runtime_error("There are no instruments entry in the ICat data base");
			 }
	
			return saveInstrumentList(response);
		}

		/**This method sets the request parameter for ICat api list isnturments
		  *@param request - reference to request object
		*/
		void CSearchHelper::setReqparamforlistInstruments(ns1__listInstruments& request)
		{
			*request.sessionId=Session::Instance().getSessionId();
		}

		/**This method saves the response parameter for ICat api list isnturments
		  *@param response - reference to response object
		  *@return API::ITableWorkspace_sptr - shared pointer to table workspace
		  */
		API::ITableWorkspace_sptr  CSearchHelper::saveInstrumentList(const ns1__listInstrumentsResponse& response)
		{			
			API::ITableWorkspace_sptr outputws =createTableWorkspace();
			outputws->addColumn("str","Instrument Name");
			try
			{			
				std::vector<std::string>::const_iterator inst_citr;
				for(inst_citr=response.return_.begin();inst_citr!=response.return_.end();++inst_citr)
				{						
					API::TableRow t = outputws->appendRow();
					// instrument  Name
					t<<*inst_citr;
				}
			}
			catch(std::runtime_error& )
			{
				throw;
			}

			return outputws;
		}

		/// This method creates table workspace
		API::ITableWorkspace_sptr CSearchHelper::createTableWorkspace()
		{
			//create table workspace
			API::ITableWorkspace_sptr outputws ;
			try
			{
				outputws=WorkspaceFactory::Instance().createTable("TableWorkspace");
			}
			catch(Mantid::Kernel::Exception::NotFoundError& )
			{
				throw std::runtime_error("Error when saving  the ICat Search Results data to Workspace");
			}
			catch(std::runtime_error)
			{
				throw std::runtime_error("Error when saving  the ICat Search Results data to Workspace");
			}
			return outputws;
		}

		int CSearchHelper::doLogout()
		{
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
				
				CErrorHandling::throwErrorMessages(icat);
			}

			ns1__logout request;
			ns1__logoutResponse response;
			boost::shared_ptr<std::string > sessionId_sptr(new std::string);
			*sessionId_sptr=Session::Instance().getSessionId();
			request.sessionId=sessionId_sptr.get();
			std::cout<<"before setting session id logout is "<<std::endl;
			request.sessionId=sessionId_sptr.get();
			std::cout<<"session id in logout is "<<*request.sessionId <<std::endl;
			int ret=icat.logout(&request,&response);
			if(ret!=0)
			{
				//icat.soap_stream_fault(std::cerr);
				CErrorHandling::throwErrorMessages(icat);
			}
			
			return ret;
		}



	}
}
