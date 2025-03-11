// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DllOpen.h"
#include "MantidKernel/Logger.h"

#if _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <string>

namespace Mantid::Kernel {

namespace {
// Static logger object
Logger g_log("DllOpen");
} // namespace

// -----------------------------------------------------------------------------
// Windows-specific implementations
// -----------------------------------------------------------------------------
#if defined(_WIN32)

const std::string LIB_SUFFIX = ".dll";

/**
 * Does the file have the expected form for this platform
 * @param filename The file name of the library
 * @return True if it matches the expected format, false otherwise
 */
bool DllOpen::isValidFilename(const std::string &filename) { return filename.ends_with(LIB_SUFFIX); }

/* Opens the Windows .dll file.
 * @param filePath :: Filepath of the library.
 * @return Pointer to library (of type void).
 **/
void *DllOpen::openDll(const std::string &filePath) {
  void *handle = LoadLibrary(filePath.c_str());
  if (!handle) {

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

    // Display the error message and exit the process
    size_t n = lstrlen((LPCTSTR)lpMsgBuf) + 40;

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, n * sizeof(TCHAR));
    _snprintf((char *)lpDisplayBuf, n, "failed with error %lu: %s", dw, (char *)lpMsgBuf);
    g_log.error() << "Could not open library " << filePath << ": " << (LPCTSTR)lpDisplayBuf << '\n';

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
  }
  return handle;
}

/* Closes an open .dll file.
 * @param handle :: A handle to the open library.
 **/
void DllOpen::closeDll(void *handle) { FreeLibrary((HINSTANCE)handle); }

#else

const std::string LIB_PREFIX = "lib";
#ifdef __linux__
const std::string LIB_SUFFIX = ".so";
#elif defined __APPLE__
const std::string LIB_SUFFIX = ".dylib";
#endif

/**
 * Does the file have the expected form for this platform
 * @param filename The file name of the library
 * @return True if it matches the expected format, false otherwise
 */
bool DllOpen::isValidFilename(const std::string &filename) {
  return filename.starts_with(LIB_PREFIX) && filename.ends_with(LIB_SUFFIX);
}

/* Opens the Linux .so file
 * @param filePath :: Filepath of the library.
 * @return Pointer to library (of type void).
 **/
void *DllOpen::openDll(const std::string &filepath) {
  void *handle = dlopen(filepath.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (!handle) {
    g_log.error("Could not open library " + filepath + ": " + dlerror());
  }
  return handle;
}

/* Closes an open .so file.
 * @param handle :: A handle to the open library.
 **/
void DllOpen::closeDll(void *handle) {
  UNUSED_ARG(handle);
  // A bug in glibc prevents us from calling this.
  // dlclose(handle);
}

#endif /* _WIN32 */

} // namespace Mantid::Kernel
