//
#include"MantidICat/Login.h"
#include "MantidICat/GSoap/stdsoap2.h"
#include "MantidKernel/MandatoryValidator.h"
#include"MantidICat/Session.h"
#include "MantidICat/ICatExport.h"

namespace Mantid
{
	namespace ICat
	{

		DECLARE_ALGORITHM(Login)
		
		void Login::init()
		{
			declareProperty("Username","", new Kernel::MandatoryValidator<std::string>(),
				"The name of the logged in user");
			declareProperty("Password","", new Kernel::MandatoryValidator<std::string>(),
				"The password of the logged in user ");
			declareProperty("DBServer","","Parameter that will identify the ICat DB server URL");
		}
		void Login::exec()
		{
			ICATPortBindingProxy icat;
			doLogin(icat);
		}
		void Login::doLogin(ICATPortBindingProxy& icat)
		{
			std::string username=getProperty("Username");
			std::string password=getProperty("Password");
						
			// Define ssl authentication scheme
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
				icat.soap_stream_fault(std::cerr);
				return;
			}

			// Login to icat
			ns1__login login;
			ns1__loginResponse loginResponse;
			
			login.username = &username;
			login.password = &password;

			std::string session_id;
			int query_id = icat.login(&login, &loginResponse);

			if( query_id == 0 )
			{
				session_id = *(loginResponse.return_);
			}
			else
			{ 
				icat.soap_stream_fault(std::cerr);
			}
			//declareProperty("SessionId","","Session id of this client session");
			//setProperty("SessionId",session_id);
			ICat::Session::Instance().setSessionId(session_id);

			//m_usermap.
			//std::map<std::string,std::string> userMap;
			//typedef std::pair <std::string, std::string> String_Pair;
			//userMap.insert(String_Pair(username,password));
						
		}
	}
}

