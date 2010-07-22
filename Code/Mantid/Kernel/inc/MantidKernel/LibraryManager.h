#ifndef MANTID_KERNEL_LIBRARY_MANAGER_H_
#define MANTID_KERNEL_LIBRARY_MANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/DllExport.h"
#include "MantidKernel/LibraryWrapper.h"

namespace Mantid
{
namespace Kernel
{
class Logger;
/** 
 Class for opening shared libraries.
 
 @author ISIS, STFC
 @date 15/10/2007
 
 Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class EXPORT_OPT_MANTID_KERNEL LibraryManagerImpl
{
public:
	//opens all suitable libraries on a given path
	int OpenAllLibraries(const std::string&, bool isRecursive=false);



private:
	friend struct Mantid::Kernel::CreateUsingNew<LibraryManagerImpl>;

	///Private Constructor
	LibraryManagerImpl();
	/// Private copy constructor - NO COPY ALLOWED
	LibraryManagerImpl(const LibraryManagerImpl&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	LibraryManagerImpl& operator = (const LibraryManagerImpl&);
	///Private Destructor
	virtual ~LibraryManagerImpl();

	///Storage for the LibraryWrappers.
	std::map< const std::string, boost::shared_ptr<Mantid::Kernel::LibraryWrapper> > OpenLibs;

	/// static reference to the logger class
	Logger& g_log;
};

///Forward declaration of a specialisation of SingletonHolder for LibraryManagerImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef __APPLE__
inline
#endif
template class EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<LibraryManagerImpl>;
typedef EXPORT_OPT_MANTID_KERNEL Mantid::Kernel::SingletonHolder<LibraryManagerImpl> LibraryManager;

} // namespace Kernel
} // namespace Mantid

#endif //MANTID_KERNEL_LIBRARY_MANAGER_H_
