#ifndef MANTID_ICAT_LOGIN_H_
#define MANTID_ICAT_LOGIN_H_

#include "MantidAPI/Algorithm.h"


namespace Mantid
{
	namespace ICat
	{
/**  Login class for logging into ICat DB .This class written as a Mantid algorithm. 
     This class uses Gsoap generated ProxyObject to connect to ICat and uses Login API .
	 
	   Required Properties:
     <UL>
     <LI> Username - The logged in user name </LI>
     <LI> Password - The password of the logged in user </LI>
     </UL>
   
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
		class DLLExport Login: public API::Algorithm
		{
		public:
			/// constructor
			Login():API::Algorithm(){}
			/// Destructor
			~Login(){}
			/// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "CatalogLogin"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }

		private:
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			
		};
	}
}
#endif
