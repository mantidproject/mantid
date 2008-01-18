#ifndef MANTID_KERNEL_LIBRARY_MANAGER_H_
#define MANTID_KERNEL_LIBRARY_MANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/System.h"
#include "MantidKernel/LibraryWrapper.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Kernel
{

// the types of the class factories
//typedef Algorithm* create_alg();
//typedef void destroy_alg(Algorithm*);

/** @class LibraryManager LibraryManager.h Kernel/LibraryManager.h

    Class for opening shared libraries.
    
    @author Matt Clarke
    @date 15/10/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
class DLLExport LibraryManager
{
public:
	static LibraryManager* Instance();
	
	virtual ~LibraryManager();
	
	//opens all suitable libraries on a given path
	int OpenAllLibraries(const std::string&, bool isRecursive=false);

private:
	///Private Constructor
	LibraryManager();
	/// Private copy constructor - NO COPY ALLOWED
	LibraryManager(const LibraryManager&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	LibraryManager& operator = (const LibraryManager&);

	///Storage for the LibraryWrappers.
	std::map< const std::string,  Mantid::Kernel::LibraryWrapper > OpenLibs;

	/// static reference to the logger class
	static Logger& g_log;

	/// Pointer to the instance
	static LibraryManager* m_instance;
};

} // namespace Kernel
} // namespace Mantid

#endif //MANTID_KERNEL_LIBRARY_MANAGER_H_
