#ifndef MANTID_ICAT_SESSION_H_
#define MANTID_ICAT_SESSION_H_

#include "MantidKernel/SingletonHolder.h"
#include "MantidICat/ICatExport.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"


namespace Mantid
{
	namespace ICat
	{
/**  SessionImpl is a singleton class responsible for saving the session id. 
     This class is used across all Mantid-ICat algorithms to get the session id 
		
    @author Sofia Antony, ISIS Rutherford Appleton Laboratory 
    @date 07/07/2010
    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */	

		class EXPORT_OPT_MANTID_ICAT SessionImpl
		{
		public:

            ///get sessionId
			const std::string& getSessionId()const{return m_sessionId;}
			/// set session id
			void setSessionId(const std::string& sessionId){m_sessionId=sessionId;}
			/// get user name
			const std::string & getUserName()const{return m_userName;}
			///set username
			void setUserName(const std::string& userName){m_userName=userName;}
			
		private:
			/// used to craete singleton
			friend struct Mantid::Kernel::CreateUsingNew<SessionImpl>;
			/// private constructor for singleton
			SessionImpl(){}
			/// private destructor
			virtual ~SessionImpl(){}
			///private  copy constructor
			SessionImpl(const SessionImpl&);
			/// private assignment
			SessionImpl operator = (const SessionImpl&);
			/// string to cache session id
			std::string m_sessionId;
			/// user name 
			std::string m_userName;


		};
#ifdef _WIN32
		// this breaks new namespace declaraion rules; need to find a better fix
		template class EXPORT_OPT_MANTID_ICAT Mantid::Kernel::SingletonHolder<SessionImpl>;
#endif /* _WIN32 */
		typedef  EXPORT_OPT_MANTID_ICAT Mantid::Kernel::SingletonHolder<SessionImpl> Session;
	}
}
#endif
