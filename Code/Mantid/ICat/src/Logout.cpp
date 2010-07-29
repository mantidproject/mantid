#include  "MantidICat/Logout.h"
#include"MantidICat/SearchHelper.h"
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
			CSearchHelper searchobj;
			searchobj.doLogout();
		}
	}
}