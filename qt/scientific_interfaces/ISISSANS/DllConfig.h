#ifndef MANTIDQT_ISISSANS_DLLCONFIG_H_
#define MANTIDQT_ISISSANS_DLLCONFIG_H_

#include "MantidKernel/System.h"

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    MantidQt CustomInterfaces library

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#ifdef IN_MANTIDQT_ISISSANS
#define MANTIDQT_ISISSANS_DLL DLLExport
#define EXTERN_MANTIDQT_ISISSANS
#else
#define MANTIDQT_ISISSANS_DLL DLLImport
#define EXTERN_MANTIDQT_ISISSANS EXTERN_IMPORT
#endif

#endif // MANTIDQT_ISISSANS_DLLCONFIG_H_
