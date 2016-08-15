#ifndef MANTID_MANTIDSCRIPTREPOSITORY_DLLCONFIG_H_
#define MANTID_MANTIDSCRIPTREPOSITORY_DLLCONFIG_H_

#include "MantidKernel/System.h"

#ifdef _WIN32
#if (IN_MANTID_SCRIPTREPO)
#define SCRIPT_DLL_EXPORT DLLExport
#else
#define SCRIPT_DLL_EXPORT DLLImport
#endif
#elif defined(__GNUC__) && !defined(__clang__)
#if (IN_MANTID_SCRIPTREPO)
#define SCRIPT_DLL_EXPORT DLLExport
#else
#define SCRIPT_DLL_EXPORT DLLImport
#endif
#else
#define SCRIPT_DLL_EXPORT
#endif

#endif // MANTID_DATAOBJECTS_DLLCONFIG_H_
