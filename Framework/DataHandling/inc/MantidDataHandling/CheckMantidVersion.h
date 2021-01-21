// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** CheckMantidVersion : Checks if the current version of Mantid is the most
  recent
*/
class DLLExport CheckMantidVersion : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  virtual std::string getVersionsFromGitHub(const std::string &url);
  virtual std::string getCurrentVersion() const;

private:
  void init() override;
  void exec() override;

  std::string cleanVersionTag(const std::string &versionTag) const;
  std::vector<int> splitVersionString(const std::string &versionString) const;
  bool isVersionMoreRecent(const std::string &localVersion, const std::string &gitHubVersion) const;
};

} // namespace DataHandling
} // namespace Mantid
