// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/ProxyInfo.h"

#include <filesystem>
#include <map>
#include <set>

namespace Mantid {

namespace DataHandling {
/** DownloadInstrument : Downloads one or more instrument files to the local
  instrument cache from the instrument repository
*/
class MANTID_DATAHANDLING_DLL DownloadInstrument : public API::Algorithm {
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
  virtual Kernel::InternetHelper::HTTPStatus doDownloadFile(const std::string &urlFile,
                                                            const std::string &localFilePath = "",
                                                            const StringToStringMap &headers = StringToStringMap());
  StringToStringMap getFileShas(const std::filesystem::path &directoryPath);
  StringToStringMap processRepository();
  std::string getValueOrDefault(const StringToStringMap &mapping, const std::string &key,
                                const std::string &defaultValue) const;

  size_t removeOrphanedFiles(const std::filesystem::path &directoryPath,
                             const std::unordered_set<std::string> &filenamesToKeep) const;

  Kernel::ProxyInfo m_proxyInfo;
};

} // namespace DataHandling
} // namespace Mantid
