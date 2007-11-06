#ifndef MANTID_KERNEL_DLLOPEN_H_
#define MANTID_KERNEL_DLLOPEN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <string>

namespace Mantid
{
namespace Kernel
{
/** @class DllOpen DllOpen.h 

    Simple class for opening shared libraries at run-time. Works for Windows and Linux.

    @author Matt Clarke
    @date 25/10/2007
    
    Copyright � 2007 ???RAL???

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
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
class DllOpen
{
public:
	/// Static method for opening the shared library
	static void* OpenDll(const std::string&);

	/// Static method for retrieving a function pointer
	static void* GetFunction(void*, const std::string&);

	/// Static method for closing the shared library
	static void CloseDll(void*);

private:
	/// Constructor private as not needed
	DllOpen() {};
	/// Copy operator private as not needed
	DllOpen(const DllOpen &a) {};
	///Destructor private as not needed	
	~DllOpen() {};
	
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DLLOPEN_H_*/
