// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidKernel/SingletonHolder.h"

#include <filesystem>
#include <set>
#include <vector>

// Forward declaration so the test class can be granted friend access without
// pulling its header into this one.
class FileFinderISISInstrumentDataCacheTest;

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
  friend class ::FileFinderISISInstrumentDataCacheTest;

public:
  std::filesystem::path getFullPath(const std::string &filename, const bool ignoreDirs = false) const;
  std::string extractAllowedSuffix(std::string &userString) const;
  /// DO NOT USE! MADE PUBLIC FOR TESTING ONLY.
  std::string makeFileName(const std::string &hintstr, const Kernel::InstrumentInfo &instrument) const;
  void setCaseSensitive(const bool cs);
  bool getCaseSensitive() const;
  static std::vector<IArchiveSearch_sptr> getArchiveSearch(const Kernel::FacilityInfo &facility);
  const API::Result<std::filesystem::path>
  findRun(const std::string &hintstr, const std::vector<std::string> &exts = {}, const bool useExtsOnly = false) const;
  std::vector<std::filesystem::path> findRuns(const std::string &hintstr, const std::vector<std::string> &exts = {},
                                              const bool useExtsOnly = false) const;
  std::vector<std::filesystem::path> findRuns(const std::vector<std::string> &hints,
                                              const std::vector<std::string> &exts = {},
                                              const bool useExtsOnly = false) const;

  /// If the hint already carries an extension, return its resolved full path
  /// in the data search dirs (or empty if not found). Returns empty for hints
  /// without an extension (which are treated as run-number hints elsewhere).
  std::filesystem::path tryResolvePathWithExtension(const std::string &filename) const;
  /// DO NOT USE! MADE PUBLIC FOR TESTING ONLY.
  const Kernel::InstrumentInfo getInstrument(const std::string &hintstr, const bool returnDefaultIfNotFound = true,
                                             const std::string &defaultInstrument = "") const;
  /// DO NOT USE! MADE PUBLIC FOR TESTING ONLY.
  std::string getExtension(const std::string &filename, const std::vector<std::string> &exts) const;
  void getUniqueExtensions(const std::vector<std::string> &extensionsToAdd, std::vector<std::string> &uniqueExts) const;
  std::pair<std::string, std::string> toInstrumentAndNumber(const std::string &hintstr,
                                                            const std::string &defaultInstrument = "") const;
  std::pair<std::string, std::string> toInstrumentAndNumber(const std::string &hintstr,
                                                            const Kernel::InstrumentInfo &instrument) const;

private:
  friend struct Mantid::Kernel::CreateUsingNew<FileFinderImpl>;

  /// Per-hint state threaded through the file-search pipeline.
  struct FileInfo {
    std::string hint{};
    bool found{false};
    std::filesystem::path path{};
    std::shared_ptr<Mantid::Kernel::InstrumentInfo> instr{};
    bool error{false};
    std::string errorMsg{};
    std::set<std::string> filenames{};
    std::vector<std::string> extensionsToSearch{};
    std::vector<Mantid::API::IArchiveSearch_sptr> archs{};
  };

  void performCacheSearch(std::vector<FileInfo> &fileInfos) const;

  /// a string that is allowed at the end of any run number
  static const std::string ALLOWED_SUFFIX;
  /// Default constructor
  FileFinderImpl();
  /// Copy constructor
  FileFinderImpl(const FileFinderImpl &);
  /// Assignment operator
  FileFinderImpl &operator=(const FileFinderImpl &);
  /// A method that returns error messages if the provided runs are invalid
  std::string validateRuns(const std::string &searchText) const;
  /// Decide whether a token the parser could not expand is a malformed run
  /// range rather than a literal file hint.
  bool isMalformedRange(const std::string &token) const;
  void prepareFileInfo(FileInfo &fileInfo, const std::vector<std::string> &extensionsProvided,
                       bool useOnlyExtensionsProvided) const;
  void processFileInfos(std::vector<FileInfo> &fileInfos, const std::vector<std::string> &extensionsProvided,
                        bool useOnlyExtensionsProvided) const;
  void performFileSearch(std::vector<FileInfo> &fileInfos) const;
  void performArchiveSearch(std::vector<FileInfo> &fileInfos) const;
  void performBatchedArchiveSearch(std::vector<FileInfo> &fileInfos, const IArchiveSearch_sptr &sharedArch) const;
  void performPerFileArchiveSearch(std::vector<FileInfo> &fileInfos) const;
  /// If every unfound FileInfo shares a single archive (and instrument) that
  /// supports batched multi-hint lookups, return that archive. Otherwise
  /// nullptr — caller falls back to per-file archive lookups.
  static IArchiveSearch_sptr batchableArchive(const std::vector<FileInfo> &fileInfos);
  const API::Result<std::filesystem::path> getISISInstrumentDataCachePath(const std::filesystem::path &cacheDir,
                                                                          const std::set<std::string> &hintstrs,
                                                                          const std::vector<std::string> &exts) const;
  const API::Result<std::filesystem::path> getArchivePath(const std::vector<IArchiveSearch_sptr> &archs,
                                                          const std::set<std::string> &hintstrs,
                                                          const std::vector<std::string> &exts) const;
  /// glob option - set to case sensitive or insensitive
  int m_globOption;
};

using FileFinder = Mantid::Kernel::SingletonHolder<FileFinderImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::FileFinderImpl>;
}
} // namespace Mantid
