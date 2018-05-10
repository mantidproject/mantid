#ifndef ALGORITHMS_REBINTOWORKSPACE_H_
#define ALGORITHMS_REBINTOWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {

namespace Algorithms {
/**
   Rebins a workspace so that the binning, for all its spectra, match that of
   the first spectrum
   of a second workspace.

   Required properties:
   <UL>
   <LI>WorkspaceToRebin - The workspace to rebin</LI>
   <LI>WorkspaceToMatch - The name of the workspace whose bin parameters are to
   be matched.</LI>
   <LI>OutputWorkspace - The name of the output workspace</LI>
   </UL>

   @author Martyn Gigg, Tessella Support Services plc
   @date 19/01/2009

   Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport RebinToWorkspace : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "RebinToWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rebin a selected workspace to the same binning as a different "
           "workspace";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Rebin"; }
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override { return {"Rebin"}; }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Create the rebin paraeters
  std::vector<double>
  createRebinParameters(Mantid::API::MatrixWorkspace_sptr toMatch);
};
} // namespace Algorithms
} // namespace Mantid

#endif /*ALGORITHMS_REBINTOWORKSPACE_H_*/
