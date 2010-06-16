#ifndef MANTID_ICAT_GETINVESTIGATION_H
#define MANTID_ICAT_GETINVESTIGATION_H


#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
namespace Mantid
{
	namespace ICat
	{
		class DLLExport CGetInvestigation:public API::Algorithm
		{
		public:
			CGetInvestigation():API::Algorithm(){}
			~CGetInvestigation(){}

		    /// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "GetInvestigation"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual const int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }

		private:
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			///
			API::ITableWorkspace_sptr doInvestigationSearch();


		};
	}
}
#endif