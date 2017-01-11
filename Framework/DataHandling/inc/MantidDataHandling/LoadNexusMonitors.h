#ifndef MANTID_DATAHANDLING_LOADNEXUSMONITORS_H_
#define MANTID_DATAHANDLING_LOADNEXUSMONITORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace DataHandling {

/** @class LoadNexusMonitors LoadNexusMonitors.h
DataHandling/LoadNexusMonitors.h

Load Monitors from NeXus files.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> Workspace - The name of the workspace to output</LI>
</UL>

@author Michael Reuter, SNS
@date October 25, 2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
class DLLExport LoadNexusMonitors : public API::Algorithm,
                                    public API::DeprecatedAlgorithm {
public:
  /// Constructor
  LoadNexusMonitors() { this->useAlgorithm("LoadNexusMonitors", 2); }

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadNexusMonitors"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load all monitors from a NeXus file into a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

protected:
  /// Intialisation code
  void init() override;

  /// Execution code
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADNEXUSMONITORS_H_ */
