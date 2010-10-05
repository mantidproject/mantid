#include  "MantidICat/Logout.h"
#include"MantidICat/SearchHelper.h"
#include "MantidICat/Session.h"
namespace Mantid
{
	namespace ICat
	{
		
		DECLARE_ALGORITHM(CLogout)

		/// Init method to declare algorithm properties
		void CLogout::init()
		{			
		}
		/// execute the algorithm
		void CLogout::exec()
		{
			//progress(m_prog, "Connecting to ICat DataBase...");
			doLogout();
		}

	  /**This method calls the ICat Logout api and disconnects from DB
		*/
		void CLogout::doLogout()
		{
			if(Session::Instance().getSessionId().empty())
			{
				throw std::runtime_error("Please login to ICat using the ICat:Login menu provided to access ICat data.");
			}
			CSearchHelper searchobj;
			searchobj.doLogout();
			Session::Instance().setSessionId("");//clearing the session id saved to Mnatid after log out
		}
	}
}
