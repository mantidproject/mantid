#ifndef DOWNLAODDATAFILE_H_
#define DOWNLAODDATAFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidICat/GSoapGenerated/soapICATPortBindingProxy.h"
#include "MantidAPI/ICatalog.h"


namespace Mantid
{
	namespace ICat
	{
/** CDownloadDataFile class is responsible for GetDataFile algorithms.
    * This algorithm  gets the location string for a given file from ISIS archive file using ICat API.
    * If the file is not able to open from isis archive,it will call another ICat api to get the URL for the file.
    * Then uses POCO http methods to download over internet.
     
	  Required Properties:
    <UL>
    <LI> Filenames - List of files to download </LI>
    <LI> InputWorkspace - The name of the workspace whioch stored the last investigation search results </LI>
	  <LI> FileLocations - List of files with location which is downloaded </LI>
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
		class DLLExport CDownloadDataFile: public API::Algorithm
		{
		public:
			/// Constructor
			CDownloadDataFile():API::Algorithm(),m_prog(0.0){}
			/// Destructor
			~CDownloadDataFile(){}
			/// Algorithm's name for identification overriding a virtual method
			virtual const std::string name() const { return "CatalogDownloadDataFiles"; }
			/// Algorithm's version for identification overriding a virtual method
			virtual int version() const { return 1; }
			/// Algorithm's category for identification overriding a virtual method
			virtual const std::string category() const { return "ICat"; }
			
		   /** This method is used for unit testing purpose.
			 * as the Poco::Net library httpget throws an exception when the nd server n/w is slow 
			 * I'm testing the download from mantid server.
			 * as the downlaod method I've written is private I can't access that in unit testing.
			 * so adding this public method to call the private downlaod method and testing.
			*/
			void testDownload(const std::string& URL,const std::string& fileName);

		private:
			/// Overwrites Algorithm method.
			void init();
			/// Overwrites Algorithm method
			void exec();
			/// get location of data file  or download method
			int doDownload( ICATPortBindingProxy & icat);
						
			/// This method is used when the download is done over internet
			void downloadFileOverInternet(const std::string& url,const std::string& fileName);

			/// If the extn of the file .raw it returns true
			bool isDataFile(const std::string& fileName);

			/// This method saves the downloaded file to disc
			void saveFiletoDisk(std::istream& rs,const std::string &fileName);

			/// This method saves downloaded file to local disk
			void doDownloadandSavetoLocalDrive(const std::string& URL,const std::string& fileName);

			/// This method replaces backwardslash with forward slashes - for linux
			void replaceBackwardSlash(std::string& inputString);

		private:
			/// progree indicator
			double m_prog;

		   

		};

	}
}
#endif
