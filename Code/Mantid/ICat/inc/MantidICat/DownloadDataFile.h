#ifndef DOWNLAODDATAFILE_H_
#define DOWNLAODDATAFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"

namespace Mantid
{
	namespace ICat
	{
		class DLLExport CDownloadDataFile: public API::Algorithm
		{
		public:
			CDownloadDataFile():API::Algorithm(){}
			~CDownloadDataFile(){}
			/// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "DownloadDataFile"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual const int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }

		private:
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			/// login method
			int doDownload( ICATPortBindingProxy & icat);
			///
			void setRequestParameters(const std::string fileName,API::ITableWorkspace_sptr ws_sptr,ns1__getDatafile& request);
			///
			void setRequestParameters(const std::string fileName,API::ITableWorkspace_sptr ws_sptr,ns1__downloadDatafile& request);
			///
			void getFileListtoDownLoad(const std::string & fileName,API::ITableWorkspace_sptr ws_sptr,
				                       std::vector<std::string>& downLoadList);
			void downloadFileOverInternet(ICATPortBindingProxy &icat,const std::vector<std::string>& fileList,API::ITableWorkspace_sptr ws_ptr);
			///
			boost::shared_ptr<std::string> m_sessionId_sptr;
			boost::shared_ptr<long long >m_fileId_sptr;

		};

	}
}
#endif
