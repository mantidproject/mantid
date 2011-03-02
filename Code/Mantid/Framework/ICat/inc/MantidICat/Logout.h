#ifndef MANTID_ICAT_LOGOUT_H_
#define MANTID_ICAT_LOGOUT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"



namespace Mantid
{
	namespace ICat
	{
	
  /** Logout class is responsible for disconnecting from  ICat DB .This class written as a Mantid algorithm. 
    
    @author Sofia Antony, STFC Rutherford Appleton Laboratory
    @date 23/07/2010
    Copyright &copy; 2010 STFC Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
		class DLLExport CLogout: public API::Algorithm
		{
		public:
			/// constructor
			CLogout():API::Algorithm(),m_prog(0){}
			/// Destructor
			~CLogout(){}
			/// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "CatalogLogout"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }

		private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			/// login method
			void doLogout();

			/// attributes
			double m_prog;
			

		};
	}
}
#endif
