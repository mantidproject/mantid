#include "MantidICat/DownloadDataFile.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidICat/ErrorHandling.h" 
#include "MantidKernel/ArrayProperty.h"

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
			
			declareProperty(new ArrayProperty<std::string> ("Filenames"),"List of filenames to download from ISIS data server");
			
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("InputWorkspace","",Direction::Input),
				"The name of the workspace which stored the last icat inevestigation search results.");
			
			declareProperty(new ArrayProperty<std::string> ("FileLocations",new NullValidator<std::vector<std::string> >,
				Direction::Output),"List of filenames downloaded from ISIS data server");
		}
		/// Execute the algorithm
		void CDownloadDataFile::exec()
		{			
			ICATPortBindingProxy icat;
			int ret=doDownload(icat);
			(void) ret;
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
				
				CErrorHandling::throwErrorMessages(icat);

			}
			//get file name
		
			std::vector<std::string> inputfiles = getProperty("Filenames");
			//get input workspace
			API::ITableWorkspace_sptr ws_sptr = getProperty("InputWorkspace");
			if(!ws_sptr)
			{
				throw std::runtime_error("Input workspace is empty");
			}
			std::vector<std::string> fileLocations;
            int ret=0;
			double counter=0.0;
			std::vector<std::string>::const_iterator citr;
			for(citr=inputfiles.begin();citr!=inputfiles.end();++citr)
			{
				std::string inputfile=*citr;

				//boolean flag used to identify the file opening permission from isis archive
				bool isis_archive=true;

				ns1__getDatafile request;

				boost::shared_ptr<std::string >sessionId_sptr(new std::string);
				request.sessionId=sessionId_sptr.get();

				boost::shared_ptr<LONG64>fileId_sptr(new LONG64 );
				request.datafileId=fileId_sptr.get();

				//m_prog=0.10;
				//progress(m_prog, "setting input params...");

				setRequestParameters(inputfile,ws_sptr,request);

				ns1__getDatafileResponse response;

				ret=icat.getDatafile(&request,&response);
				m_prog =(double(++counter)/double(inputfiles.size()))/2.0;
				
				progress(m_prog, "getting the location string from isis archive...");
				if(ret==0)
				{
					if(response.return_->location)
					{
						std::string fileloc=*response.return_->location;
						// the file location string format is something like 
						// \\isis\inst$\Instruments$\NDXMERLIN\Instrument\data\cycle_07_3\MER00601.raw 

						replaceBackwardSlash(fileloc);
						//if we are able to open the file from the location returned by the file location
						//the user got the permission to acess isis archive
						std::ifstream isisfile(fileloc.c_str());
						if(!isisfile)
						{
							isis_archive=false;
						}
						else
						{	
							//setProperty("FileLocation",fileloc);
							g_log.information()<<"isis archive location for the selected file is "<<fileloc<<std::endl;
							fileLocations.push_back(fileloc);
							setProperty("FileLocations",fileLocations);
							
						}

					}
					else
					{
						isis_archive=false;
					}
				}
				else
				{				
					isis_archive=false;
				}
				if(!isis_archive)
				{
					g_log.information()<<"File can not be opened from isis archive,calling ICat API to download from data.isis server"<<std::endl;
					std::vector<std::string> fileList;
					std::string runNumber;
					//if the file is already downloded don't down load it again.
					//This is bcoz if a raw/nexus file is selected to download log files associated also I'm downloading.
					//if the list contains log files names which is already downlaoded below check stops it from downloading again.
                    if(isFileDownloaded(inputfile,fileLocations))
					{
						continue;
					}
					
					//get the name of all files associated with raw/nexus file to download
					if(isDataFile(inputfile))
					{
						getRunNumberfromFileName(inputfile,runNumber);

						getFileListtoDownLoad(runNumber,ws_sptr,fileList);
					}
					else //if not a raw/nexus file just download only the selected file
					{
						fileList.push_back(inputfile);
					}
					//download the files from the server to the machine where mantid is installed
					downloadFileOverInternet(icat,fileList,ws_sptr);

					//get the download directory name
					std::string downloadedFName( Kernel::ConfigService::Instance().getString("defaultsave.directory"));
					downloadedFName+=inputfile;
					//replace "\" with "/"
					replaceBackwardSlash(downloadedFName);
					//set the filelocation property
					//setProperty("FileLocation",downloadFileName);
					fileLocations.push_back(downloadedFName);
					setProperty("FileLocations",fileLocations);
				}
			}
			m_prog =1.0;
			progress(m_prog,"saving the location string to mantid...");
			return ret;

		}
		/** This method checks the file is already downled by looking at the file name in downlaoded file list.
		 * @param fileName -name of the file to download
		 * @param downloadedList - vector containing list of files downloaded.
		*/
		bool CDownloadDataFile::isFileDownloaded(const std::string& fileName,std::vector<std::string>& downloadedList )
		{
			std::vector<std::string>::const_iterator citr;
			citr=find(downloadedList.begin(),downloadedList.end(),fileName);
			if(citr!=downloadedList.end())
			{
				return true;
			}
			return false;

		}
		/* *This method calls ICat API downloadDatafile and gets the URL string and uses the URL to down load file server
		 * @param icat -Icat proxy object
		 * @param fileList - vector containing list of files to download.
		*/
		void CDownloadDataFile::downloadFileOverInternet(ICATPortBindingProxy &icat,const std::vector<std::string>& fileList,
			API::ITableWorkspace_sptr ws_sptr)
		{	
			double counter =0.0;
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
				// get the URL using ICAT API
				int ret=icat.downloadDatafile(&request,&response);
				
				m_prog +=(double(++counter)/double(fileList.size()))/2.0;
				progress(m_prog, "downloading the file from data.isis server...");
				if(ret!=0)
				{
					CErrorHandling::throwErrorMessages(icat);
				}
				if(!response.URL)
				{
					throw std::runtime_error("Empty URL returned from ICat databse");
				}
				//g_log.error()<<"URL returned  is "<<*response.URL<<std::endl;

				//download using Poco HttpClient session and save to local disk
				doDownloadandSavetoLocalDrive(*response.URL,*citr);
			}//end of for loop for download file list iteration

		}

	  /** This method sets the request parameters for the downloadDatafile api
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
			int row=0;const int col=0;
			long long fileId=0;
			try
			{
				ws_sptr->find(fileName,row,col);
				fileId=ws_sptr->cell<long long >(row,col+2);
			}
			catch(std::range_error&)
			{
				throw std::runtime_error("selected file "+fileName+" not exists in the ICat search results workspace");
			}
			catch(std::out_of_range&)
			{
				throw std::runtime_error("selected file "+fileName+" not exists in the ithe ICat search results workspace");;
			}
			catch(std::runtime_error&)
			{
				throw std::runtime_error("selected file "+fileName+" not exists in the the ICat search results workspace");;
			}

			if(Session::Instance().getSessionId().empty())
			{
				throw std::runtime_error("Please login to ICat using the ICat:Login menu provided to access ICat data.");
			}

			//get the sessionid which is cached in session class during login
			*request.sessionId=Session::Instance().getSessionId();
			*request.datafileId=fileId;

		}

	  /** This method sets the input request parameters.
		* @param fileName - name of the file to download
		* @param ws_sptr - shared pointer to workspace
		* @param request - request object
		*/
		void CDownloadDataFile::setRequestParameters(const std::string fileName,API::ITableWorkspace_sptr ws_sptr,ns1__downloadDatafile& request)
		{			
			//look up  filename in the table workspace
			int row=0;const int col=0;long long fileId=0;
			ws_sptr->find(fileName,row,col);
			try
			{
				fileId=ws_sptr->cell<long long >(row,col+2);
			}
			catch(std::range_error&)
			{
				throw std::runtime_error("selected file "+fileName+" not exists in the input ICat search results workspace");;
			}
			catch(std::out_of_range&)
			{
				throw std::runtime_error("selected file "+fileName+" not exists in the input ICat search results workspace");;
			}
			catch(std::runtime_error&)
			{
				throw std::runtime_error("selected file "+fileName+" not exists in the input ICat search results workspace");
			}
			if(Session::Instance().getSessionId().empty())
			{
				throw std::runtime_error("Please login to ICat using the ICat:Login menu provided to access ICat data.");
			}
			
			*request.sessionId=Session::Instance().getSessionId();
			*request.datafileId=fileId;//m_fileId_sptr.get();

		}

	   /** This method returns the file names(row file and log file list) to down load from the selected row file name.
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

	   /** This method returns the run number associated to a file from teh file name
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
				for (int i=0;i<=static_cast<int>(indexDot-1);++i)
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

	   /** This method checks the file extn and if it's a raw file reurns true
		* This is useful when the we download a file over internet and save to local drive,
		* to open the file in binary or ascii mode
		* @param fileName  file name
		*/
		bool CDownloadDataFile::isDataFile(const std::string & fileName)
		{			
			std::basic_string <char>::size_type dotIndex;
			//const std::basic_string <char>::size_type npos = -1;
			//find the position of .in row file
			dotIndex = fileName.find_last_of (".");
			std::string fextn=fileName.substr(dotIndex+1,fileName.size()-dotIndex);

			bool binary;
			(!fextn.compare("raw")|| !fextn.compare("RAW")|| !fextn.compare("nxs")|| !fextn.compare("NXS")) ? binary = true : binary = false;
			return binary;

		}

	   /** This method downloads file over internet using Poco HTTPClientSession 
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
					throw std::runtime_error("URL string is empty,ICat interface can not download the file"+fileName);
				}
				start=clock();

				HTTPClientSession session(uri.getHost(), uri.getPort());
				HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
				session.sendRequest(req);
				
				HTTPResponse res;
				std::istream& rs = session.receiveResponse(res);
				clock_t end=clock();
		        float diff = float(end - start)/CLOCKS_PER_SEC;
				g_log.information()<<"Time taken to download file "<< fileName<<" is "<<std::fixed << std::setprecision(2) << diff <<" seconds" << std::endl;
				//save file to local disk
				saveFiletoDisk(rs,fileName);

			}
			catch(Poco::SyntaxException&)
			{
			 throw std::runtime_error("Error when downloading the data file"+ fileName);
			}
			catch(Poco::Exception&)
			{			   
				
				throw std::runtime_error("Can not download the file "+fileName +". Path is invalid for the file.");
			}

		}

	  /** This method saves the input stream to a file
		* @param rs input stream
		* @param fileName name of the output file
		*/
		void CDownloadDataFile::saveFiletoDisk(std::istream& rs,const std::string& fileName)
		{			
			std::string filepath ( Kernel::ConfigService::Instance().getString("defaultsave.directory"));
			filepath += fileName;
			
			std::ios_base::openmode mode;
			//if raw/nexus file open it in binary mode else ascii 
			isDataFile(fileName)? mode = std::ios_base::binary : mode = std::ios_base::out;
			std::ofstream ofs(filepath.c_str(), mode);
			if ( ofs.rdstate() & std::ios::failbit )
			{
				throw Mantid::Kernel::Exception::FileError("Error on creating File",fileName);
			}
			//copy the input stream to a file.
			StreamCopier::copyStream(rs, ofs);

		}

	   /** This method is used for unit testing purpose.
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
		/** This method replaces backward slash with forward slash for linux compatibility.
		 * @param inputString input string
  		 */
		void CDownloadDataFile::replaceBackwardSlash(std::string& inputString)
		{
			std::basic_string <char>::iterator iter;
			for(iter=inputString.begin();iter!=inputString.end();++iter)
			{
				if((*iter)=='\\')
				{
					(*iter)='/';
				}
			}

		}


	}
}
