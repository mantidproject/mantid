// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    DataHandling library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_DATAHANDLING
#define MANTID_DATAHANDLING_DLL DLLExport
#else
#define MANTID_DATAHANDLING_DLL DLLImport
#endif /* IN_MANTID_DATAHANDLING*/
