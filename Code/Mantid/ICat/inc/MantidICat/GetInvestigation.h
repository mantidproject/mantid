#ifndef MANTID_ICAT_GETINVESTIGATION_H_
#define MANTID_ICAT_GETINVESTIGATION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

/** CGetDataFiles is a class responsible for GetInvestigation algorithm.
  * This algorithm does the basic search and returns the investigations record

    Required Properties:
    <UL>
    <LI> InvestigationId - The id of the investigation to display</LI>
    <LI> InputWorkspace -  Input workspace which saved last search</LI>
	<LI> OutputWorkspace - The putput workspace to store  </LI>
    </UL>
   
    @author Sofia Antony, STFC Rutherford Appleton Laboratory
    @date 07/07/2010
    Copyright &copy; 2010 STFC Rutherford Appleton Laboratory

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
		class DLLExport CGetDataFiles:public API::Algorithm
		{
		public:

			/// Constructor
			CGetDataFiles():API::Algorithm(){}
			/// Destructor
			~CGetDataFiles(){}

		    /// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "GetInvestigation"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual const int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }

		private:
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			/// do datafile search
			API::ITableWorkspace_sptr doDataFilesSearch();


		};
	}
}
#endif