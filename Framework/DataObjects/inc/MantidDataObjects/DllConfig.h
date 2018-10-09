// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_DLLCONFIG_H_
#define MANTID_DATAOBJECTS_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    DataObjects library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_DATAOBJECTS
#define MANTID_DATAOBJECTS_DLL DLLExport
#define EXTERN_MANTID_DATAOBJECTS
#else
#define MANTID_DATAOBJECTS_DLL DLLImport
#define EXTERN_MANTID_DATAOBJECTS EXTERN_IMPORT
#endif /* IN_MANTID_DATAOBJECTS*/

#endif // MANTID_DATAOBJECTS_DLLCONFIG_H_
