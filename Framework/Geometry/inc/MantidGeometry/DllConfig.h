// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_DLLCONFIG_H_
#define MANTID_GEOMETRY_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    Geometry library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_GEOMETRY
#define MANTID_GEOMETRY_DLL DLLExport
#define EXTERN_MANTID_GEOMETRY
#else
#define MANTID_GEOMETRY_DLL DLLImport
#define EXTERN_MANTID_GEOMETRY EXTERN_IMPORT
#endif /* IN_MANTID_GEOMETRY*/

#endif // MANTID_GEOMETRY_DLLCONFIG_H_
