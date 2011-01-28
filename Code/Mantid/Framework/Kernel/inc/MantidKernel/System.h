#ifndef MANTID_KERNEL_SYSTEM_H_
#define MANTID_KERNEL_SYSTEM_H_

/*  A system-wide file to contain, e.g., useful system-dependent macros

    @author Russell Taylor, Tessella Support Services plc
    @date 26/10/2007
    
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


/**
 * Definitions of the DLLImport and DLLExport compiler directives for MSVC
 */
#ifdef _WIN32
  #pragma warning( disable: 4251 )
  // MG: Given that we are compiling everything with msvc under Windows and linking all with the same runtime we can disable the warning about
  // inheriting from a non-exported interface, e.g. std::runtime_error
  #pragma warning( disable : 4275 )
  #define DLLExport __declspec( dllexport )
  #define DLLImport __declspec( dllimport )
#else
  #define DLLExport
  #define DLLImport
#endif

// information for holding onto stdint.h
#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#ifdef _WIN32
typedef signed char          int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short     uint16_t;
typedef int                 int32_t;
typedef unsigned           uint32_t;
typedef long long           int64_t;
typedef unsigned long long uint64_t;
#else
typedef signed char      int8_t;
typedef unsigned char   uint8_t;
typedef short           int16_t;
typedef unsigned short uint16_t;
typedef int             int32_t;
typedef unsigned int   uint32_t;
typedef long            int64_t;
typedef unsigned long  uint64_t;
#endif
#endif


#include <string>

namespace Mantid 
{

  /// Return what we consider to be an empty integer, -INT_MAX
  DLLExport int EMPTY_INT();

  /// Return what we consider to be an empty double, -DBL_MAX
  DLLExport double EMPTY_DBL();

namespace Kernel
{

  /** This class is simply used in the subscription of classes into the various
   *  factories in Mantid. The fact that the constructor takes an int means that
   *  the comma operator can be used to make a call to the factories' subscribe
   *  method in the first part.
   */
  class DLLExport RegistrationHelper
  {
  public:
    /// Constructor. Does nothing.
    /// @param i :: Takes an int
    RegistrationHelper(int i); // Implemented in cpp file to kill compiler warning
  };

  //Return the executable path
  DLLExport std::string getDirectoryOfExecutable();
  
  //Return the full path to the executable
  DLLExport std::string getPathToExecutable();

  //Check if the path is on a network drive
  DLLExport bool isNetworkDrive(const std::string & path);

  /// Return the name corresponding to the mangled string given by typeid
  DLLExport std::string getUnmangledTypeName(const std::type_info& type);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_SYSTEM_H_*/
