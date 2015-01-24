#ifndef MANTID_DATAHANDLING_LoadPreNexus_H_
#define MANTID_DATAHANDLING_LoadPreNexus_H_

#include <string>
#include <vector>
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/System.h"

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
  LoadPreNexus();
  virtual ~LoadPreNexus();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load a collection of PreNexus files.";
  }

  virtual int version() const;
  virtual const std::string category() const;
  void parseRuninfo(const std::string &runinfo, std::string &dataDir,
                    std::vector<std::string> &eventFilenames);
  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  void init();
  void exec();
  void runLoadNexusLogs(const std::string &runinfo, const std::string &dataDir,
                        const double prog_start, const double prog_stop);
  void runLoadMonitors(const double prog_start, const double prog_stop);

  API::IEventWorkspace_sptr m_outputWorkspace;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LoadPreNexus_H_ */
