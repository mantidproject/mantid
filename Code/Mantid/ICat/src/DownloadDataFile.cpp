#include "MantidICat/DownloadDataFile.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include <fstream>
#include <iomanip>
//
//
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

		void CDownloadDataFile::init()
		{
			declareProperty("Filename","","Name of the file to download");
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("InputWorkspace","",Direction::Input),
				"The name of the workspace which stored the last icat file search result");

		}
		void CDownloadDataFile::exec()
		{
			ICATPortBindingProxy icat;
			int ret=doDownload(icat);
		}
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
				icat.soap_stream_fault(std::cerr);

			}
			//get file name
			std::string inputfile=getPropertyValue("Filename");
			//get input workspace
			API::ITableWorkspace_sptr ws_sptr=getProperty("InputWorkspace");
            ns1__getDatafile request;
			setRequestParameters(inputfile,ws_sptr,request);
			ns1__getDatafileResponse response;
			bool isis_archive=false;
			int ret=icat.getDatafile(&request,&response);
			if(ret==0)
			{
				if(response.return_->location)
				{
					std::string fileloc=*response.return_->location;
					std::cout<<"Filelocation is "<<fileloc<<std::endl;

					// the file location string contains some extra at the beggining of the string.
					//removing the extra string "file://" from the file location string.
					std::basic_string<char>::size_type index=fileloc.find_first_of("f");
					const std::basic_string <char>::size_type npos = -1;
					if(index!=npos)
				    {
					 fileloc.erase(index,7);
				    }
					//if we are able to open the file from the location returned by the file location
					std::ifstream isisfile(fileloc.c_str());
					if(!isisfile)
				   {
						isis_archive=false;
				    }
					else
				    {
						//
						//need to call loadraw
				    }

				}
			}
			else
			{
				icat.soap_stream_fault(std::cerr);
			}
			if(!isis_archive)
			{

				std::string filepath="C:\\Mantid\\Code\\Mantid\\ICat\\test\\";
				std::vector<std::string> fileList;
				fileList.push_back(inputfile);
                //get the name of all log files associated with raw file to down load
				getFileListtoDownLoad(inputfile,ws_sptr,fileList);

				//std::vector<std::string>::const_iterator citr;
				//for(citr=fileList.begin();citr!=fileList.end();++citr)
				//{
				//	//call Icat API download data file.
				//	ns1__downloadDatafile request;
				//	//set request parameters
				//	setRequestParameters(*citr,ws_sptr,request);
				//	ns1__downloadDatafileResponse response;
				//	std::string URL;
				//	// download data file.
				//	ret=icat.downloadDatafile(&request,&response);
				//	if(ret==0)
				//	{
				//		if(response.URL)
				//		{
				//			URL=*response.URL;
				//		}
				//		//std::cout<<"URL is "<<URL<<std::endl;
				//	}
		  //		   //use HTTP  Get method to download the data file from the server to local machine
				//	try
				//	{
				//		URI uri(URL);
				//		std::string path(uri.getPathAndQuery());
				//		if (path.empty()) path = "/";

				//		HTTPClientSession session(uri.getHost(), uri.getPort());
				//		HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
				//		session.sendRequest(req);
				//		HTTPResponse res;
				//		clock_t start=clock();
				//		std::istream& rs = session.receiveResponse(res);
				//		clock_t end=clock();
				//		float diff = float(end -start)/CLOCKS_PER_SEC;
				//		std::cout<< "Time taken  for http download over internet is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;

				//		filepath+=*citr;
				//		std::ofstream ofs( filepath.c_str(), std::ios_base::out );
				//		if ( ofs.rdstate() & std::ios::failbit )
				//		{
				//			std::cout<<"Could not open file"<<std::endl;
				//			return 0;
				//		}
				//		StreamCopier::copyStream(rs, ofs);
				//		filepath.clear();
				//	}
				//	catch(Poco::Exception& )
				//	{
				//		throw;
				//	}
				//}//end of for loop for download file list iteration
				downloadFileOverInternet(icat,fileList,ws_sptr);
			}
			return ret;

		}
		void CDownloadDataFile::downloadFileOverInternet(ICATPortBindingProxy &icat,const std::vector<std::string>& fileList,
			                    API::ITableWorkspace_sptr ws_sptr)
		{
			    std::string filepath="C:\\Mantid\\Code\\Mantid\\ICat\\test\\";

				std::vector<std::string>::const_iterator citr;
				for(citr=fileList.begin();citr!=fileList.end();++citr)
				{
					//call Icat API download data file.
					ns1__downloadDatafile request;
					//set request parameters
					setRequestParameters(*citr,ws_sptr,request);
					ns1__downloadDatafileResponse response;
					std::string URL;
					// download data file.
					int ret=icat.downloadDatafile(&request,&response);
					if(ret==0)
					{
						if(response.URL)
						{
							URL=*response.URL;
						}
						//std::cout<<"URL is "<<URL<<std::endl;
					}
		  		   //use HTTP  Get method to download the data file from the server to local machine
					try
					{
						URI uri(URL);
						std::string path(uri.getPathAndQuery());
						if (path.empty()) path = "/";

						HTTPClientSession session(uri.getHost(), uri.getPort());
						HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
						session.sendRequest(req);
						HTTPResponse res;
						clock_t start=clock();
						std::istream& rs = session.receiveResponse(res);
						clock_t end=clock();
						float diff = float(end -start)/CLOCKS_PER_SEC;
						std::cout<< "Time taken  for http download over internet is "<<std::fixed << std::setprecision(2) << diff << " seconds" << std::endl;

						filepath+=*citr;
						std::ofstream ofs( filepath.c_str(), std::ios_base::out );
						if ( ofs.rdstate() & std::ios::failbit )
						{
							std::cout<<"Could not open file"<<std::endl;
							return ;
						}
						StreamCopier::copyStream(rs, ofs);
						filepath.clear();
					}
					catch(Poco::Exception& )
					{
						throw;
					}
				}//end of for loop for download fuile list iteration

		}

		/* *This method sets the request parameters for the downloadDatafile api
		   *This method takes filename and table workspace as input and do a look up for
		   *filename in the table  and gets the corresponding file id for the filename.
		   *The file id is the request parameter for the downloadDatafile api.
		   *@param fileName name of teh file to download
		   *@param ws_sptr shared workspace pointer
		   *@param request input request object

		*/
		void CDownloadDataFile::setRequestParameters(const std::string fileName,API::ITableWorkspace_sptr ws_sptr,ns1__getDatafile& request)
		{
			//std::string fileName=getProperty("Filename");
			//API::ITableWorkspace_sptr ws_sptr=getProperty("InputWorkspace");
			//write code to look up this filename in the table workspace
			int row=0;const int col=1;
			long long fileId=0;
			try
			{
			ws_sptr->find(fileName,row,col);
			fileId=ws_sptr->cell<long long >(row,col+2);
			}
			catch(std::range_error&)
			{
				throw;
			}
			catch(std::runtime_error&)
			{
				throw;
			}

			//get the sessionid which is cached in session class during login
			m_sessionId_sptr=boost::shared_ptr<std::string >(new std::string);
			*m_sessionId_sptr=Session::Instance().getSessionId();
			request.sessionId=m_sessionId_sptr.get();
			//m_fileId_sptr=boost::shared_ptr<LONG64>(new LONG64);
			*m_fileId_sptr=fileId;
			request.datafileId=m_fileId_sptr.get();

		}
		void CDownloadDataFile::setRequestParameters(const std::string fileName,API::ITableWorkspace_sptr ws_sptr,ns1__downloadDatafile& request)
		{

			//look up this filename in the table workspace
			int row=0;const int col=1;long long fileId=0;
			ws_sptr->find(fileName,row,col);
			try
			{
				fileId=ws_sptr->cell<long long >(row,col+2);
			}
			catch(std::range_error&)
			{
				throw;
			}
			catch(std::runtime_error&)
			{
				throw;
			}

			//get the sessionid which is cached in session class during login
			m_sessionId_sptr=boost::shared_ptr<std::string >(new std::string);
			*m_sessionId_sptr=Session::Instance().getSessionId();
			request.sessionId=m_sessionId_sptr.get();
			//m_fileId_sptr=boost::shared_ptr<long long >(new long long );
			*m_fileId_sptr=fileId;
			request.datafileId=m_fileId_sptr.get();

		}
		/* This method returns the file names(row file and log file list) to down load from the selected row file name.
		*/
		void CDownloadDataFile::getFileListtoDownLoad(const std::string & fileName,API::ITableWorkspace_sptr ws_sptr,
			                    std::vector<std::string>& downLoadList )
		{
			//std::string fileName=getProperty("Filename");
			//API::ITableWorkspace_sptr ws_sptr=getProperty("InputWorkspace");
			//find the run number from row file name.
			std::basic_string <char>::size_type indexDot;
			const std::basic_string <char>::size_type npos = -1;
			std::string runNumber;
			//find the position of . in row file
			indexDot = fileName.find_last_of ( "." );
			if(indexDot!= npos)
			{
				for (int i=0;i<=indexDot-1;++i)
				{
				int inteqv=(int)fileName.at(i);
				if( (inteqv>=65 && inteqv<=90) || (inteqv>=97 && inteqv<=122) )
				{
				}
				else
				{
					runNumber+=fileName.at(i);
				}

				}
			}

			ColumnVector<std::string> fileNameCol = ws_sptr->getVector("Name");
			int colSize=fileNameCol.size();
			for (int i=0;i<colSize;++i)
			{
				if(fileNameCol[i].find(runNumber)!=npos)
				{
					std::cout<<"download list adding file "<<fileNameCol[i]<<std::endl;
					downLoadList.push_back(fileNameCol[i]);
				}

			}

		}
	}
}
