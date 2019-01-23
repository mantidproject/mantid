// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_DLLCONFIG_H_
#define MANTID_API_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    API library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_API
#define MANTID_API_DLL DLLExport
#define EXTERN_MANTID_API
#else
#define MANTID_API_DLL DLLImport
#define EXTERN_MANTID_API EXTERN_IMPORT
#endif /* IN_MANTID_API*/

#endif // MANTID_API_DLLCONFIG_H_
