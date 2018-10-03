// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LoadPreNexus_H_
#define MANTID_DATAHANDLING_LoadPreNexus_H_

#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/System.h"
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {

/** LoadPreNexus : Workflow algorithm to load a collection of preNeXus files.

  @date 2012-01-30
*/
class DLLExport LoadPreNexus : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load a collection of PreNexus files.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadEventPreNexus", "LoadPreNexusMonitors", "LoadNexus"};
  }
  const std::string category() const override;
  void parseRuninfo(const std::string &runinfo, std::string &dataDir,
                    std::vector<std::string> &eventFilenames);
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;
  void runLoadNexusLogs(const std::string &runinfo, const std::string &dataDir,
                        const double prog_start, const double prog_stop);
  void runLoadMonitors(const double prog_start, const double prog_stop);

  API::IEventWorkspace_sptr m_outputWorkspace;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LoadPreNexus_H_ */
