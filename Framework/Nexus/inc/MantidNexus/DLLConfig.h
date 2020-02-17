// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUS_DLLCONFIG_H_
#define MANTID_NEXUS_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    NeXus library

    @author Lamar Moore, STFC
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_NEXUS
#define MANTID_NEXUS_DLL DLLExport
#define EXTERN_MANTID_NEXUS
#else
#define MANTID_NEXUS_DLL DLLImport
#define EXTERN_MANTID_NEXUS EXTERN_IMPORT
#endif /* IN_MANTID_NEXUS*/

#endif // MANTID_NEXUS_DLLCONFIG_H_
