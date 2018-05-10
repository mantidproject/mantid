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

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
