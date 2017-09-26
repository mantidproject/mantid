#ifndef MANTID_TYPES_DLLCONFIG_H_
#define MANTID_TYPES_DLLCONFIG_H_

/*
This file contains the DLLExport/DLLImport linkage configuration for the
Types library

@author Lamar Moore, STFC

Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// Header only dependency on this file
// Dependency on Kernel should NOT be added
#include "../../Kernel/inc/MantidKernel/System.h"

#ifdef IN_MANTID_TYPES
#define MANTID_TYPES_DLL DLLExport
#define EXTERN_MANTID_TYPES
#else
#define MANTID_TYPES_DLL DLLImport
#define EXTERN_MANTID_TYPES extern
#endif /* IN_MANTID_KERNEL*/

#endif // MANTID_KERNEL_DLLCONFIG_H_
