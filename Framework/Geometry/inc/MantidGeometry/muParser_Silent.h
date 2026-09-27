// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**

  This file includes the muParser.h header file and avoids a conflict with the
  MANTID_GEOMETRY_DLL macro
  that we both have defined.
*/
#ifdef _WIN32
#ifdef MANTID_GEOMETRY_DLL
#undef MANTID_GEOMETRY_DLL // Avoid warning about redefinition
#endif
#include "MantidGeometry/DllConfig.h"

#include <muParser.h>
#undef MANTID_GEOMETRY_DLL
#define MANTID_GEOMETRY_DLL __declspec(dllexport) // Our version.
#else
#include <muParser.h>
#endif
