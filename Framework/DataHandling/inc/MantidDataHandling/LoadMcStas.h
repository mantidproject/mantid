#ifndef MANTID_DATAHANDLING_LOADMCSTAS_H_
#define MANTID_DATAHANDLING_LOADMCSTAS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/System.h"

#include "MantidAPI/WorkspaceGroup_fwd.h"

namespace Mantid {
namespace DataHandling {

/** LoadMcStas : TODO: DESCRIPTION

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadMcStas : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a McStas NeXus file into an workspace.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadMcStasNexus", "LoadNexus"};
  }
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;

  API::WorkspaceGroup_sptr
  groupWorkspaces(const std::vector<std::string> &workspaces) const;

  std::vector<std::string>
  readEventData(const std::map<std::string, std::string> &eventEntries,
                ::NeXus::File &nxFile);
  std::vector<std::string>
  readHistogramData(const std::map<std::string, std::string> &histogramEntries,
                    ::NeXus::File &nxFile);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LoadMcStas_H_ */
