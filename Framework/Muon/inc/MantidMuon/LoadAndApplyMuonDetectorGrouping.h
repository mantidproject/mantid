#ifndef MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPING_H_
#define MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPING_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/System.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

namespace Mantid {
namespace Muon {

/** LoadAndApplyMuonDetectorGrouping :

@date 2018-05-31

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport LoadAndApplyMuonDetectorGrouping : public API::Algorithm {
public:
  /// (Empty) Constructor
  LoadAndApplyMuonDetectorGrouping() : API::Algorithm() {}
  /// Virtual destructor
  ~LoadAndApplyMuonDetectorGrouping() {}
  /// Algorithm's name
  const std::string name() const override {
    return "LoadAndApplyMuonDetectorGrouping";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Grouping;Transforms\\Grouping";
  }
  /// Algorithm's summary for identification
  const std::string summary() const override {
    return "Load a file containing grouping/pairing infromation (XML format) "
           "and apply the grouping and pairing analysis to the input "
           "workspace.";
  }
  /// See also
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess;ApplyMuonDetectorGrouping"};
  }
  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;
  /// Allow WorkspaceGroup property to function correctly.
  bool checkGroups() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  API::Grouping loadGroupsAndPairs();

  // Add all the supplied groups to the ADS, inside wsGrouped, by
  // executing the ApplyMuonDetectorGrouping algorithm
  void addGroupingToADS(const Mantid::Muon::AnalysisOptions &options,
                        Mantid::API::Workspace_sptr ws,
                        Mantid::API::WorkspaceGroup_sptr wsGrouped);

  // The pairing algorithm is not yet implemented
  void addPairingToADS(const Mantid::Muon::AnalysisOptions &options,
                       Mantid::API::Workspace_sptr ws,
                       Mantid::API::WorkspaceGroup_sptr wsGrouped);

  void addGroupingInformationToADS(const Mantid::API::Grouping &grouping);

  /// Sets some default options for grouping algorithm.
  Mantid::Muon::AnalysisOptions setDefaultOptions();
};

} // namespace Muon
} // namespace Mantid

#endif
