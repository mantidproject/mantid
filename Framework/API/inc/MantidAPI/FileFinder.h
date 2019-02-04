// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_FILEFINDER_H_
#define MANTID_API_FILEFINDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidKernel/SingletonHolder.h"

#include <set>
#include <vector>

namespace Mantid {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
namespace Kernel {
class InstrumentInfo;
class FacilityInfo;
} // namespace Kernel
namespace API {

/**
This class finds data files given an instrument name (optionally) and a run
number

@author Roman Tolchenov, Tessella plc
@date 23/07/2010
*/
class MANTID_API_DLL FileFinderImpl {
public:
  std::string getFullPath(const std::string &filename,
                          const bool ignoreDirs = false) const;
  std::string getPath(const std::vector<IArchiveSearch_sptr> &archs,
                      const std::set<std::string> &filenames,
                      const std::vector<std::string> &exts) const;
  /// DO NOT USE! MADE PUBLIC FOR TESTING ONLY.
  std::string makeFileName(const std::string &hint,
                           const Kernel::InstrumentInfo &instrument) const;
  void setCaseSensitive(const bool cs);
  bool getCaseSensitive() const;
  std::vector<IArchiveSearch_sptr>
  getArchiveSearch(const Kernel::FacilityInfo &facility) const;
  std::string findRun(const std::string &hintstr,
                      const std::vector<std::string> &exts = {},
                      const bool useExtsOnly = false) const;
  std::vector<std::string> findRuns(const std::string &hintstr,
                                    const std::vector<std::string> &exts = {},
                                    const bool useExtsOnly = false) const;
  /// DO NOT USE! MADE PUBLIC FOR TESTING ONLY.
  const Kernel::InstrumentInfo getInstrument(const std::string &hint) const;
  /// DO NOT USE! MADE PUBLIC FOR TESTING ONLY.
  std::string getExtension(const std::string &filename,
                           const std::vector<std::string> &exts) const;
  void getUniqueExtensions(const std::vector<std::string> &extensionsToAdd,
                           std::vector<std::string> &uniqueExts) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<FileFinderImpl>;

  /// a string that is allowed at the end of any run number
  static const std::string ALLOWED_SUFFIX;
  /// Default constructor
  FileFinderImpl();
  /// Copy constructor
  FileFinderImpl(const FileFinderImpl &);
  /// Assignment operator
  FileFinderImpl &operator=(const FileFinderImpl &);
  std::string extractAllowedSuffix(std::string &userString) const;
  std::pair<std::string, std::string>
  toInstrumentAndNumber(const std::string &hint) const;
  std::string getArchivePath(const std::vector<IArchiveSearch_sptr> &archs,
                             const std::set<std::string> &filenames,
                             const std::vector<std::string> &exts) const;
  std::string toUpper(const std::string &src) const;
  /// glob option - set to case sensitive or insensitive
  int m_globOption;
};

using FileFinder = Mantid::Kernel::SingletonHolder<FileFinderImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::FileFinderImpl>;
}
} // namespace Mantid

#endif // MANTID_API_FILEFINDER_H_
