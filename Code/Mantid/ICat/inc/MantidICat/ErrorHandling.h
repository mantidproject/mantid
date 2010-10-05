#ifndef MANTID_ICAT_ERRORHANDLING_H_
#define MANTID_ICAT_ERRORHANDLING_H_

#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"

/** CErrorHandling class responsible for handling errors in Mantid-ICat Algorithms.
  * This algorithm  gives the datsets for a given investigations record
       
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
namespace Mantid
{
	namespace ICat
	{
		class CErrorHandling
		{
		public:
			/// Constructors
			CErrorHandling(){}
			/// Destructors
			~CErrorHandling(){}
			
			/* *This method throws the error string returned by gsoap to mantid upper layer
			   *@param iact -ICat proxy object 
			*/
			static void throwErrorMessages(ICATPortBindingProxy& icat)
			{
				char buf[600];
				const int len=600;
				icat.soap_sprint_fault(buf,len);
				std::string error(buf);
				std::string begmsg("<message>");
				std::string endmsg("</message>");
				
				std::basic_string <char>::size_type index1=error.find(begmsg);
				std::basic_string <char>::size_type index2=error.find(endmsg);
				std::string exception;
				if(index1!=std::string::npos && index2!=std::string::npos)
				{	
					exception = error.substr(index1+begmsg.length(),index2-(index1+begmsg.length()));
				}
				throw std::runtime_error(exception);
			}

		};
	}
}
#endif
