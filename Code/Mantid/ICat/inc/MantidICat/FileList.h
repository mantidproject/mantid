#ifndef MANTID_ICAT_FILELIST_H_
#define MANTID_ICAT_FILELIST_H_


#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid
{
	namespace ICat
	{
		class DLLExport CFileList:public API::Algorithm
		{
		public:

			///conctructor
			CFileList():API::Algorithm(){}
			/// Destructor
			~CFileList(){}

		    /// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "Login"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }

		private:
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			/// search for files
			API::ITableWorkspace_sptr doFileSearch();


		};
	}
}
#endif
