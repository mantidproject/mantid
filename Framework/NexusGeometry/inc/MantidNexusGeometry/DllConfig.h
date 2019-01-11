// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_DLLCONFIG_H_
#define MANTID_NEXUSGEOMETRY_DLLCONFIG_H_

/** DLLConfig : Import Export configurations for Nexus Geometry
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
