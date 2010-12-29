#ifndef MANTID_KERNEL_DLLOPEN_H_
#define MANTID_KERNEL_DLLOPEN_H_

#include <string>

#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{
class Logger;

/** @class DllOpen DllOpen.h 

 Simple class for opening shared libraries at run-time. Works for Windows and Linux.

 @author ISIS, STFC
 @date 25/10/2007
 
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
class DLLExport DllOpen
{
public:
	/// Static method for opening the shared library
	static void* OpenDll(const std::string&);

	/// Static method for opening the shared library
	static void* OpenDll(const std::string&, const std::string&);

	/// Static method for retrieving a function pointer
	static void* GetFunction(void*, const std::string&);

	/// Static method for closing the shared library
	static void CloseDll(void*);

	/// Static method for converting a filename to a libName (without lib___.so or ___.dll)
	static const std::string ConvertToLibName(const std::string&);

    /// Adds a directiry to the dll search path.
    static void addSearchDirectory(const std::string&);

private:
	/// Constructor private as not needed
	DllOpen()
	{};
	/// Copy operator private as not needed
	DllOpen(const DllOpen &)
	{};
	///Destructor private as not needed	
	~DllOpen()
	{};

	//private functions specific to implementation
	/// Implementation specifc static method for opening a shared library
	static void* OpenDllImpl(const std::string&);

	/// Implementation specifc static method for retrieving a function pointer
	static void* GetFunctionImpl(void*, const std::string&);

	/// Implementation specifc static method for closing a shared library
	static void CloseDllImpl(void*);

    /// Implementation specifc static method for adding a directiry to the dll search path.
    static void addSearchDirectoryImpl(const std::string&);

	/// Static reference to the logger class
	static Mantid::Kernel::Logger& log;

	///lib prefix
	static const std::string LIB_PREFIX;
	///lib postfix
	static const std::string LIB_POSTFIX;
	///path seperator 
	static const std::string PATH_SEPERATOR;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DLLOPEN_H_*/
