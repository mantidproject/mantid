#ifndef MANTID_KERNEL_LIBRARY_WRAPPER_H_
#define MANTID_KERNEL_LIBRARY_WRAPPER_H_

#include <string>

#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{

/** @class LibraryWrapper LibraryWrapper.h Kernel/LibraryWrapperr.h

 Class for wrapping a shared library.
 
 @author ISIS, STFC
 @date 10/01/2008
 
 Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport LibraryWrapper
{
public:
	LibraryWrapper();
	virtual ~LibraryWrapper();

	//Returns true if DLL is opened or already open
	bool OpenLibrary(const std::string&);

	bool OpenLibrary(const std::string&, const std::string&);

private:
	/** An untyped pointer to the loaded library.
	 * This is created and deleted by this class.
	 **/
	void* module;
};

} // namespace Kernel
} // namespace Mantid

#endif //MANTID_KERNEL_LIBRARY_WRAPPER_H_
