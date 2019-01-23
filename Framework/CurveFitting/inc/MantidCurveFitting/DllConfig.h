// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_DLLCONFIG_H_
#define MANTID_CURVEFITTING_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    DataHandling library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_CURVEFITTING
#define MANTID_CURVEFITTING_DLL DLLExport
#define EXTERN_MANTID_CURVEFITTING
#else
#define MANTID_CURVEFITTING_DLL DLLImport
#define EXTERN_MANTID_CURVEFITTING EXTERN_IMPORT
#endif /* IN_MANTID_CURVEFITTING*/

#endif // MANTID_CURVEFITTING_DLLCONFIG_H_
