#ifndef MANTID_ICAT_GETINVESTIGATION_H_
#define MANTID_ICAT_GETINVESTIGATION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"


namespace Mantid
{
	namespace ICat
	{
 /**CGetDataFiles is a class responsible for GetInvestigation algorithm.
    This algorithm does the basic search and returns the investigations record
    Required Properties:
    <UL>
    <LI> InvestigationId - The id of the investigation to display</LI>
    <LI> InputWorkspace -  Input workspace which saved last search</LI>
	  <LI> OutputWorkspace - The putput workspace to store  </LI>
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

		class DLLExport CGetDataFiles:public API::Algorithm
		{
		public:

			/// Constructor
			CGetDataFiles():API::Algorithm(){}
			/// Destructor
			~CGetDataFiles(){}

		    /// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "CatalogGetDataFiles"; }
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
			/// do datafile search
			API::ITableWorkspace_sptr doDataFilesSearch();
			/// This method returns true if  the given file name is raw file or nexus file otherwise false
			bool isDataFile(const std::string& fileName);

			/// This method filters the log files from the workspace
			void filterLogFiles(API::ITableWorkspace_sptr& ws_sptr);


		};
	}
}
#endif
