// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ProxyInfo.h"

#include <map>
#include <set>

namespace Mantid {

namespace DataHandling {
/** DownloadInstrument : Downloads one or more instrument files to the local
  instrument cache from the instrument repository
*/
class DLLExport DownloadInstrument : public API::Algorithm {
public:
  DownloadInstrument();

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadInstrument", "UpdateScriptRepository"}; }
  const std::string category() const override;
  const std::string summary() const override;

protected:
  // Convenience typedef
  using StringToStringMap = std::map<std::string, std::string>;

private:
  void init() override;
  void exec() override;
  virtual int doDownloadFile(const std::string &urlFile, const std::string &localFilePath = "",
                             const StringToStringMap &headers = StringToStringMap());
  StringToStringMap getFileShas(const std::string &directoryPath);
  const std::string getDownloadableRepoUrl(const std::string &filename) const;
  StringToStringMap processRepository();
  std::string getValueOrDefault(const StringToStringMap &mapping, const std::string &key,
                                const std::string &defaultValue) const;

  size_t removeOrphanedFiles(const std::string &directoryPath,
                             const std::unordered_set<std::string> &filenamesToKeep) const;

  Kernel::ProxyInfo m_proxyInfo;
};

} // namespace DataHandling
} // namespace Mantid
