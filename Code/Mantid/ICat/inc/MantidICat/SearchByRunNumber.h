#ifndef MADNTID_ICAT_CSEARCHBYRUNNUMBER_H
#define MANTID_ICAT_CSEARCHBYRUNNUMBER_H

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
namespace Mantid
{
	namespace ICat
	{
		class DLLExport CSearchByRunNumber: public API::Algorithm
		{
		public:
			///constructor
			CSearchByRunNumber():API::Algorithm(){}
			///destructor
			~CSearchByRunNumber()
			{
			}
			/// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "SearchByRunNumber"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual const int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }
		private:
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			
			/// search method 
			API::ITableWorkspace_sptr  doSearchByRunNumber();

									
		};
	}
}

#endif