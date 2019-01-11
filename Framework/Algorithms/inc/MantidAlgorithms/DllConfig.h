// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_DLLCONFIG_H_
#define MANTID_ALGORITHMS_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    DataHandling library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_ALGORITHMS
#define MANTID_ALGORITHMS_DLL DLLExport
#else
#define MANTID_ALGORITHMS_DLL DLLImport
#endif /* IN_MANTID_ALGORITHMS*/

#endif // MANTID_ALGORITHMS_DLLCONFIG_H_
