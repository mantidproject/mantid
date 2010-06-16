#ifndef MANTID_ICAT_SESSION_H
#define MANTID_ICAT_SESSION_H

#include "MantidKernel/SingletonHolder.h"
#include "MantidICat/ICatExport.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"
namespace Mantid
{
	namespace ICat
	{
		class EXPORT_OPT_MANTID_ICAT SessionImpl
		{
		public:

			const std::string& getSessionId()const{return m_sessionId;}
			void setSessionId(const std::string& sessionId){m_sessionId=sessionId;}

		private:
			friend struct Mantid::Kernel::CreateUsingNew<SessionImpl>;
			SessionImpl(){}
			virtual ~SessionImpl(){}
			///private  copy constructor
			SessionImpl(const SessionImpl&);
			/// private assignment
			SessionImpl operator = (const SessionImpl&);
			/// string to cache session id
			std::string m_sessionId;

			///
			ICATPortBindingProxy m_icat;

		};
#ifdef _WIN32
		// this breaks new namespace declaraion rules; need to find a better fix
		template class EXPORT_OPT_MANTID_ICAT Mantid::Kernel::SingletonHolder<SessionImpl>;
#endif /* _WIN32 */
		typedef  EXPORT_OPT_MANTID_ICAT Mantid::Kernel::SingletonHolder<SessionImpl> Session;
	}
}
#endif
