#ifndef MANTID_NEXUSGEOMETRY_DLLCONFIG_H_
#define MANTID_NEXUSGEOMETRY_DLLCONFIG_H_

/** DLLConfig : Import Export configurations for Nexus Geometry

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_NEXUS_GEOMETRY
#define MANTID_NEXUSGEOMETRY_DLL DLLExport
#define EXTERN_MANTID_NEXUSGEOMETRY
#else
#define MANTID_NEXUSGEOMETRY_DLL DLLImport
#define EXTERN_MANTID_NEXUSGEOMETRY EXTERN_IMPORT
#endif /* IN_MANTID_NEXUSGEOMETRY*/

#endif // MANTID_NEXUSGEOMETRY_DLLCONFIG_H_
