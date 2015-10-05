#ifndef MANTID_ALGORITHMS_CALMUONDETECTORPHASES_H_
#define MANTID_ALGORITHMS_CALMUONDETECTORPHASES_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
namespace Mantid {
namespace Algorithms {

/** CalMuonDetectorPhases : Calculates asymmetry and phase for each spectra in a
  workspace

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CalMuonDetectorPhases : public API::Algorithm {
public:
  /// Default constructor
  CalMuonDetectorPhases() : API::Algorithm(){};
  /// Destructor
  virtual ~CalMuonDetectorPhases(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CalMuonDetectorPhases"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculates the asymmetry and phase for each detector in a "
           "workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Muon"; }

private:
  /// Initialise the algorithm
  void init();
  /// Execute the algorithm
  void exec();
  /// Validate the inputs
  std::map<std::string, std::string> validateInputs();
  /// Prepare workspace for fit
  API::MatrixWorkspace_sptr
  prepareWorkspace(const API::MatrixWorkspace_sptr &ws, double startTime,
                   double endTime);
  /// Fit the workspace
  void fitWorkspace(const API::MatrixWorkspace_sptr &ws, double freq,
                    std::string groupName, API::ITableWorkspace_sptr &resTab,
                    API::WorkspaceGroup_sptr &resGroup);
  /// Create the fitting function as string
  std::string createFittingFunction(int nspec, double freq);
  /// Extract asymmetry and phase from fitting results
  API::ITableWorkspace_sptr
  extractDetectorInfo(const API::ITableWorkspace_sptr &paramTab, size_t nspec);
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALMUONDETECTORPHASES_H_ */