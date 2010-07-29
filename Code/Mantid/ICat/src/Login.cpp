//
#include "MantidICat/Login.h"
#include "MantidICat/GSoap/stdsoap2.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidICat/Session.h"
#include "MantidICat/ICatExport.h"
#include "MantidICat/ErrorHandling.h"
#include "MantidKernel/MaskedProperty.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;

		DECLARE_ALGORITHM(Login)

		/// Init method to declare algorithm properties
		void Login::init()
		{
			declareProperty("Username","", new Kernel::MandatoryValidator<std::string>(),
				"The name of the logged in user");
			declareProperty(new MaskedProperty<std::string>("Password","",new Kernel::MandatoryValidator<std::string>()),
				"The password of the logged in user ");

			//declareProperty("DBServer","","Parameter that will identify the ICat DB server URL");
		}
		/// execute the algorithm
		void Login::exec()
		{
			progress(m_prog, "Connecting to ICat DataBase...");
			ICATPortBindingProxy icat;
			doLogin(icat);
		}

	  /**This method calls the ICat the login api and connects to DB and returns session id.
		*@param icat ICat proxy object
		*/
		void Login::doLogin(ICATPortBindingProxy& icat)
		{
			m_prog=0.2;
			std::string username=getProperty("Username");
			std::string password=getProperty("Password");
			progress(m_prog, "User Name and Password...");

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
				//icat.soap_stream_fault(std::cerr);
				CErrorHandling::throwErrorMessages(icat);
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
				m_prog=0.8;
				progress(m_prog, "Session Id Obtained...");
				//save session id
				ICat::Session::Instance().setSessionId(session_id);
				//save user name
				ICat::Session::Instance().setUserName(username);
			}
			else
			{ 
				CErrorHandling::throwErrorMessages(icat);
				return;
			}
			m_prog=1.0;
			progress(m_prog, "Login Succesful...");


		}
	}
}

