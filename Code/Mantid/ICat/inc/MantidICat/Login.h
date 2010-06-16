#ifndef MANTID_ICAT_LOGIN_H
#define MANTID_ICAT_LOGIN_H


#include "MantidAPI/Algorithm.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"

namespace Mantid
{
	namespace ICat
	{
	
		class DLLExport Login: public API::Algorithm
		{
		public:
			/// 
			Login():API::Algorithm(){}
			~Login(){}
			/// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "Login"; }
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
			void doLogin( ICATPortBindingProxy & icat);
			

		};
	}
}
#endif
