#include "MantidICat/DownloadDataFile.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidICat/ErrorHandling.h" 

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include <fstream>
#include <iomanip>

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::Exception;

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CDownloadDataFile)

		/// declaring algorithm properties 
		void CDownloadDataFile::init()
		{
			declareProperty("Filename","","Name of the file to download");
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("InputWorkspace","",Direction::Input),
				"The name of the workspace which stored the last icat search result");
		}
		/// Execute the algorithm
		void CDownloadDataFile::exec()
		{
			ICATPortBindingProxy icat;
			int ret=doDownload(icat);
		}

	  /**This method gets the location string for the selected file from isis archive using Icat api.
		* If the location string is not avalable,It calls another api to down load the file over internet.
		* @param icat - reference to icatproxy object
		*/
		int CDownloadDataFile::doDownload(ICATPortBindingProxy &icat)
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
				//icat.soap_stream_fault(std::cerr);
				CErrorHandling::throwErrorMessages(icat);

			}
			//get file name
			std::string inputfile=getPropertyValue("Filename");
			//get input workspace
			API::ITableWorkspace_sptr ws_sptr=getProperty("InputWorkspace");

			//boolean flag used to identify the file opening permission from isis archive
			bool isis_archive=true;

			ns1__getDatafile request;

			boost::shared_ptr<std::string >sessionId_sptr(new std::string);
			request.sessionId=sessionId_sptr.get();

			boost::shared_ptr<LONG64>fileId_sptr(new LONG64 );
			request.datafileId=fileId_sptr.get();

			setRequestParameters(inputfile,ws_sptr,request);

			ns1__getDatafileResponse response;

			int ret=icat.getDatafile(&request,&response);
			if(ret==0)
			{
				if(response.return_->location)
				{
					std::string fileloc=*response.return_->location;
					//std::cout<<"file location is "<<fileloc<<std::endl;

					// the file location string contains some extra at the beggining of the string.
					//removing the extra string "file://" from the file location string.
					std::basic_string<char>::size_type index=fileloc.find_first_of("f");
					const std::basic_string <char>::size_type npos = -1;
					if(index!=npos)
					{
						fileloc.erase(index,7);
					}
					//if we are able to open the file from the location returned by the file location
					//the user got the permission to acess isis archive
					std::ifstream isisfile(fileloc.c_str());
					if(!isisfile)
					{
						isis_archive=false;
					}
					else
					{
						//
						//if(isRawFile(fileName))
						//{
      //                    //need to call loadraw
						//}
						//else if (isNexusFile(fileName))
						//{
						//	//need to call load nexus.
						//}

					}

				}
				else
				{
					isis_archive=false;
				}
			}
			else
			{
				//icat.soap_stream_fault(std::cerr);
				isis_archive=false;
			}
			if(!isis_archive)
			{
				std::vector<std::string> fileList;
				//fileList.push_back(inputfile);
				std::string runNumber;
				getRunNumberfromFileName(inputfile,runNumber);
				//get the name of all files associated with raw file to download
				getFileListtoDownLoad(runNumber,ws_sptr,fileList);
				//download the files from the server to the machine where mantid is installed
				downloadFileOverInternet(icat,fileList,ws_sptr);
			}
			return ret;

		}
		/* This method calls ICat API downloadDatafile and gets the URL string and uses the URL to down load file server
		 * @param icat -Icat proxy object
		 * @param fileList - vector containing list of files to download.
		*/
		void CDownloadDataFile::downloadFileOverInternet(ICATPortBindingProxy &icat,const std::vector<std::string>& fileList,
			API::ITableWorkspace_sptr ws_sptr)
		{						
			std::vector<std::string>::const_iterator citr;
			for(citr=fileList.begin();citr!=fileList.end();++citr)
			{
				//call Icat API download data file.
				ns1__downloadDatafile request;

				boost::shared_ptr<std::string >sessionId_sptr(new std::string);
				request.sessionId=sessionId_sptr.get();

				boost::shared_ptr<LONG64>fileId_sptr(new LONG64 );
				request.datafileId=fileId_sptr.get();

				//set request parameters
				setRequestParameters(*citr,ws_sptr,request);

				ns1__downloadDatafileResponse response;
				std::string URL;
				// get the URL using ICAT API
				int ret=icat.downloadDatafile(&request,&response);
				if(ret!=0)
				{
					CErrorHandling::throwErrorMessages(icat);
				}
				
				if(response.URL)
				{
					URL=*response.URL;
				}

				//download using Poco HttpClient session and save to local disk
				doDownloadandSavetoLocalDrive(URL,*citr);
			}//end of for loop for download file list iteration

		}

	/*  * This method sets the request parameters for the downloadDatafile api
		* This method takes filename and table workspace as input and do a look up for
		* filename in the table  and gets the corresponding file id for the filename.
		* The file id is the request parameter for the downloadDatafile api.
		* @param fileName name of the file to download
		* @param ws_sptr shared workspace pointer
		* @param request input request object
		*/
		void CDownloadDataFile::setRequestParameters(const std::string fileName,API::ITableWorkspace_sptr ws_sptr,
			ns1__getDatafile& request)
		{			
			//write code to look up this filename in the table workspace
			int row=0;const int col=2;
			long long fileId=0;
			try
			{
				//i think find is not required now.
				ws_sptr->find(fileName,row,col);
				fileId=ws_sptr->cell<long long >(row,col+2);
			}
			catch(std::range_error&)
			{
				throw;
			}
			catch(std::out_of_range&)
			{
				throw;
			}
			catch(std::runtime_error&)
			{
				throw;
			}

			//get the sessionid which is cached in session class during login
			//m_sessionId_sptr=boost::shared_ptr<std::string >(new std::string);
			//*m_sessionId_sptr=Session::Instance().getSessionId();
			//m_fileId_sptr=boost::shared_ptr<long long >(new long long );
			//*m_fileId_sptr=fileId;

			*request.sessionId=Session::Instance().getSessionId();
			*request.datafileId=fileId;
			//std::cout<<"data file id is "<<*request.datafileId<<std::endl;

		}

	  /** This method sets the input request parameters.
		* @param fileName - name of the file to download
		* @param ws_sptr - shared pointer to workspace
		* @param request - request object
		*/
		void CDownloadDataFile::setRequestParameters(const std::string fileName,API::ITableWorkspace_sptr ws_sptr,ns1__downloadDatafile& request)
		{			
			//look up  filename in the table workspace
			int row=0;const int col=2;long long fileId=0;
			ws_sptr->find(fileName,row,col);
			try
			{
				fileId=ws_sptr->cell<long long >(row,col+2);
			}
			catch(std::range_error&)
			{
				throw;
			}
			catch(std::out_of_range&)
			{
				throw;
			}
			catch(std::runtime_error&)
			{
				throw;
			}
			
			*request.sessionId=Session::Instance().getSessionId();
			*request.datafileId=fileId;//m_fileId_sptr.get();

		}

	   /* This method returns the file names(row file and log file list) to down load from the selected row file name.
		* @param runNumber - the run number associated to the row file
		* @param ws_sptr - shared pointer to workspace
		* @param downLoadList list of files to download
		*/
		void CDownloadDataFile::getFileListtoDownLoad(const std::string & runNumber,const API::ITableWorkspace_sptr& ws_sptr,
			std::vector<std::string>& downLoadList )
		{	

			const std::basic_string <char>::size_type npos = -1;
			ColumnVector<std::string> fileNameCol = ws_sptr->getVector("Name");
			int colSize=fileNameCol.size();
			for (int i=0;i<colSize;++i)
			{
				if(fileNameCol[i].find(runNumber)!=npos)
				{
					downLoadList.push_back(fileNameCol[i]);
				}

			}

		}

	   /* This method returns the run number associated to a file from teh file name
		* @param fileName  file name
		* @param runNumber runnumber 
		*/
		void CDownloadDataFile::getRunNumberfromFileName(const std::string& fileName, std::string& runNumber)
		{
			//get  the run number from row file name.
			std::basic_string <char>::size_type indexDot;
			const std::basic_string <char>::size_type npos = -1;
			//find the position of .in row file
			indexDot = fileName.find_last_of (".");
			if(indexDot!= npos)
			{
				for (int i=0;i<=indexDot-1;++i)
				{
					int intval=(int)fileName.at(i);
					if( (intval>=65 && intval<=90) || (intval>=97 && intval<=122) )
					{
					}
					else
					{
						runNumber+=fileName.at(i);
					}

				}
			}

		}

	   /* This method checks the file extn and if it's a raw file reurns true
		* This is useful when the we download a file over internet and save to local drive,
		* to open the file in binary or ascii mode
		* @param fileName  file name
		*/
		bool CDownloadDataFile::isBinaryFile(const std::string & fileName)
		{			
			std::basic_string <char>::size_type dotIndex;
			const std::basic_string <char>::size_type npos = -1;
			//find the position of .in row file
			dotIndex = fileName.find_last_of (".");
			std::string fextn=fileName.substr(dotIndex+1,fileName.size()-dotIndex);

			bool binary;
			(!fextn.compare("raw")|| !fextn.compare("RAW")|| !fextn.compare("nxs")) ? binary = true : binary = false;
			//std::cout<<"file opening mode  for filename "<<fileName<<"is "<<binary<<std::endl;
			return binary;

		}

	   /* This method downloads file over internet using Poco HTTPClientSession 
		* @param URL- URL of the file to down load
		* @param fileName  file name
		*/
		void CDownloadDataFile::doDownloadandSavetoLocalDrive(const std::string& URL,const std::string& fileName)
		{
			clock_t start;
			//use HTTP  Get method to download the data file from the server to local disk
			try
			{
				URI uri(URL);
				std::string path(uri.getPathAndQuery());
				if (path.empty()) 
				{
					throw std::runtime_error("Invalid Path when downloading the data file");
				}
				start=clock();

				HTTPClientSession session(uri.getHost(), uri.getPort());
				HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
				session.sendRequest(req);
				
				HTTPResponse res;
				std::istream& rs = session.receiveResponse(res);
				clock_t end=clock();
		        float diff = float(end - start)/CLOCKS_PER_SEC;
				g_log.debug()<<"Time taken to download file "<< fileName<<"is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;
				//save file to local disk
				saveFiletoDisk(rs,fileName);

			}
			catch(Poco::SyntaxException& e)
			{
				g_log.error()<<e.what()<<std::endl;
				throw;
			}
			catch(Poco::Exception& ex )
			{			   
				g_log.error()<<ex.what()<<std::endl;
				throw;
			}

		}

	  /** This method saves the input stream to a file
		* @param rs input stream
		* @param fileName name of the output file
		*/
		void CDownloadDataFile::saveFiletoDisk(std::istream& rs,const std::string& fileName)
		{
			
			std::string filepath = Kernel::ConfigService::Instance().getString("icatDownload.directory");
			//std::cout<<"ICatDownload path is "<<filepath<<std::endl;
			filepath += fileName;
			std::ios_base::openmode mode;
			//if raw/nexus file open it in binary mode else ascii 
			isBinaryFile(fileName)? mode = std::ios_base::binary : mode = std::ios_base::out;
			std::ofstream ofs(filepath.c_str(), mode);
			if ( ofs.rdstate() & std::ios::failbit )
			{
				throw Mantid::Kernel::Exception::FileError("Error on creating File",fileName);
			}
			//copy the input stream to a file.
			StreamCopier::copyStream(rs, ofs);
		}

	   /* This method is used for unit testing purpose.
		* as the Poco::Net library httpget throws an exception when the nd server n/w is slow 
		* I'm testing the download from mantid server.
		* as the downlaod method I've written is private I can't access that in unit testing.
		* so adding this public method to call the private downlaod method and testing.
		* @param URL - URL of the file to download
		* @param fileName - name of the file
		*/
		void CDownloadDataFile::testDownload(const std::string& URL,const std::string& fileName)
		{
			doDownloadandSavetoLocalDrive(URL,fileName);

		}


	}
}
