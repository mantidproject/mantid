#ifndef REGEXSUPPORT_H
#define REGEXSUPPORT_H

#include "MantidKernel/DllConfig.h"
#ifndef Q_MOC_RUN
#include <boost/regex.hpp>
#endif

namespace Mantid {
namespace Kernel {
namespace Strings {
/// Find if a pattern matches a string
template <typename T>
MANTID_KERNEL_DLL int StrComp(const char * /*Text*/, const boost::regex & /*Re*/, T & /*Aout*/,
                              const int  /*compNum*/= 0);

/// Find if a pattern matches
template <typename T>
MANTID_KERNEL_DLL int StrComp(const std::string & /*Text*/, const boost::regex & /*Re*/, T & /*Aout*/,
                              const int  /*compNum*/= 0);

/// Find is a pattern matches
MANTID_KERNEL_DLL int StrLook(const char * /*Sx*/, const boost::regex & /*Re*/);
/// Find is a pattern matches
MANTID_KERNEL_DLL int StrLook(const std::string & /*Text*/, const boost::regex & /*Re*/);

/// Split  a line into component parts
MANTID_KERNEL_DLL std::vector<std::string> StrParts(std::string /*Sdx*/,
                                                    const boost::regex & /*Re*/);

/// Split  a line searched parts
template <typename T>
int StrFullSplit(const std::string & /*text*/, const boost::regex & /*Re*/, std::vector<T> & /*Aout*/);
/// Split  a line searched parts
template <typename T>
int StrFullSplit(const char *, const boost::regex &, std::vector<T> &);
/// Split  a line searched parts
template <typename T>
int StrSingleSplit(const std::string & /*text*/, const boost::regex & /*Re*/, std::vector<T> & /*Aout*/);
/// Split  a line searched parts
template <typename T>
int StrSingleSplit(const char *, const boost::regex &, std::vector<T> &);

/// Cut out the searched section and returns component
template <typename T>
int StrFullCut(std::string & /*Text*/, const boost::regex & /*Re*/, T & /*Aout*/, const int  /*compNum*/= -1);
/// Cut out the searched section and returns component
template <typename T>
int StrFullCut(std::string & /*Text*/, const boost::regex & /*Re*/, std::vector<T> & /*Aout*/);

/// Extract a section from a string
MANTID_KERNEL_DLL int StrRemove(std::string & /*Sdx*/, std::string & /*Extract*/,
                                const boost::regex & /*Re*/);

/// Find a compmonent in a Regex in a file
template <typename T>
MANTID_KERNEL_DLL int findComp(std::istream & /*fh*/, const boost::regex & /*Re*/, T & /*Out*/);

/// Finds a pattern in a file
MANTID_KERNEL_DLL int findPattern(std::istream & /*fh*/, const boost::regex & /*Re*/,
                                  std::string & /*Out*/);

} // NAMESPACE Strings

} // NAMESPACE Kernel

} // NAMESPACE Mantid
#endif
