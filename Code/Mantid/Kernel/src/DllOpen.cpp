/** 
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

#include <string>
#include <iostream>

/**
     If the OS is Windows then LoadLibrary, GetProcAddress and FreeLibrary are used. 
     Some casting to HINSTANCE is required.
     Shared library name is of the form *.dll. 
     
     If the OS is Linux then dlopen, dlsym and dlclose are used.
     Shared library name is of the form lib*.so.
     
*/

#if _WIN32

#include "windows.h"
#include "MantidKernel/DllOpen.h"

namespace Mantid
{
namespace Kernel
{

void* DllOpen::OpenDll(const std::string& libName)
{
	std::string str = libName + ".dll";
	return LoadLibrary(str.c_str());
}

void* DllOpen::GetFunction(void* lib, const std::string& funcName)
{
	return GetProcAddress((HINSTANCE)lib, funcName.c_str());
}

void DllOpen::CloseDll(void* lib)
{
	FreeLibrary((HINSTANCE)lib);
}

#else

#include "/usr/include/dlfcn.h"
#include "MantidKernel/DllOpen.h"

namespace Mantid
{
namespace Kernel
{

void* DllOpen::OpenDll(const std::string& libName)
{
	std::string str = "lib" + libName + ".so";
	return dlopen(str.c_str(), RTLD_NOW);
}

void* DllOpen::GetFunction(void* lib, const std::string& funcName)
{
	return dlsym(lib, funcName.c_str());
}

void DllOpen::CloseDll(void* lib)
{
	dlclose(lib);
}

#endif

} // namespace Kernel
} // namespace Mantid
