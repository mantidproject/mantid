#ifndef MANTID_DATAHANDLING_LOADMCSTAS_H_
#define MANTID_DATAHANDLING_LOADMCSTAS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"

#include "MantidAPI/WorkspaceGroup.h"

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
  LoadMcStas();
  virtual ~LoadMcStas();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads a McStas NeXus file into an workspace.";
  }

  virtual int version() const;
  virtual const std::string category() const;

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::NexusDescriptor &descriptor) const;

private:
  void init();
  void exec();

  void readEventData(const std::map<std::string, std::string> &eventEntries,
                     API::WorkspaceGroup_sptr &outputGroup,
                     ::NeXus::File &nxFile);
  void
  readHistogramData(const std::map<std::string, std::string> &histogramEntries,
                    API::WorkspaceGroup_sptr &outputGroup,
                    ::NeXus::File &nxFile);

  // used as part of given useful names to workspaces added to output
  // groupworkspace
  size_t m_countNumWorkspaceAdded;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LoadMcStas_H_ */
