#ifndef LOADPRENEXUSMONITORS_H_
#define LOADPRENEXUSMONITORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace DataHandling {
/** @class Mantid::DataHandling::LoadPreNeXusMonitors

    A data loading routine for SNS PreNeXus beam monitor (histogram) files.

    @author Stuart Campbell, SNS ORNL
    @date 20/08/2010

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
class DLLExport LoadPreNexusMonitors : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  LoadPreNexusMonitors();
  /// Algorithm's name
  const std::string name() const override { return "LoadPreNexusMonitors"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This is a routine to load in the beam monitors from SNS preNeXus "
           "files into a workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadEventPreNexus", "LoadPreNexus"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\PreNexus";
  }
  /// Algorithm's aliases
  const std::string alias() const override { return "LoadPreNeXusMonitors"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Number of monitors
  int nMonitors;

  /// Set to true when instrument geometry was loaded.
  bool instrument_loaded_correctly;

  void runLoadInstrument(const std::string &instrument,
                         API::MatrixWorkspace_sptr localWorkspace);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*LOADPRENEXUSMONITORS_H_*/
