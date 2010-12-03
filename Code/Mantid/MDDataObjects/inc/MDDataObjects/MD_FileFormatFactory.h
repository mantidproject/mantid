#ifndef MD_FILE_FORMAT_FACTORY_H
#define MD_FILE_FORMAT_FACTORY_H
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MDDataObjects/IMD_FileFormat.h"
// existing file formats
#include "MDDataObjects/MD_File_hdfV1.h"
#include "MDDataObjects/MD_File_hdfMatlab.h"
#include "MDDataObjects/MD_File_hdfMatlab4D.h"
/** The class takes the file name and returns file the reader/writer which would understand and interpret the file format 
 *  of the file, provided. If the file is not found, the factory returns the default reader or the one, modified 
 *
 *  The file reader has to satisfy IMD_FileFormat interface
 *
 * not a factory in Mantid sence, as the rules to select file format are defined within the class, so user who creates new format
 * has to add these rules to the class manualy 

    @author Alex Buts, RAL ISIS
    @date 03/12/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
namespace MDDataObjects
{
	/** the enum describes the request, a user can make to ask for a particular file reader 
	 *  we are not suppose to handle very complex requests as the file reader should be selected
	 *  acdording to the file format
	*/
	enum user_request{
		best_fit,  //< default file reader
		test_data,  //< the reader, which does not reads the file, but returns fake data for rebinning;
		old_4DMatlabReader //< compartibility function, will be probably deleted in a future;
	};

class DLLExport MD_FileFormatFactory
{
public:
	/** function returns the file reader which would understand the file format of the file,
	 *  defined by the file name provided.
	 *
	 *  If the file has not been found, the function assumes the 
	 *  new file and returns defauld file reader, bound to this new file
	 */
	static std::auto_ptr<IMD_FileFormat> getFileReader(const char *fileName,user_request rec=best_fit);
private:
	// singleton holder
	static MD_FileFormatFactory *pFactory;
	// default constructor and others;
	MD_FileFormatFactory();
	~MD_FileFormatFactory();
	MD_FileFormatFactory(const MD_FileFormatFactory &);
	MD_FileFormatFactory & operator= (const MD_FileFormatFactory &);

   /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& f_log;

	IMD_FileFormat* select_file_reader(const char *file_name,user_request rec=best_fit);
};
} // end namespaces
}
#endif