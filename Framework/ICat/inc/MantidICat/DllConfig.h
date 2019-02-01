// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ICAT_DLLCONFIG_H_
#define MANTID_ICAT_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    ICAT library

    @author Jay Rainey, ISIS Rutherford Appleton Laboratory
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_ICAT
#define MANTID_ICAT_DLL DLLExport
#define EXTERN_MANTID_ICAT
#else
#define MANTID_ICAT_DLL DLLImport
#define EXTERN_MANTID_ICAT EXTERN_IMPORT
#endif /* IN_MANTID_ICAT */

#endif // MANTID_ICAT_DLLCONFIG_H_
