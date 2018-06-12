#ifndef MANTID_ALGORITHMS_REBIN_H_
#define MANTID_ALGORITHMS_REBIN_H_

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/** Takes a workspace as input and rebins the data according to the input rebin
   parameters.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the workspace to take as input. Must
   contain histogram data. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result. </LI>
    <LI> RebinParameters - The new bin boundaries in the form
   X1,deltaX1,X2,deltaX2,X3,... </LI>
    </UL>

    The algorithms used in the VectorHelper::rebin() and
   VectorHelper::createAxisFromRebinParams()
    are based on the algorithms used in OPENGENIE /src/transform_utils.cxx
   (Freddie Akeroyd, ISIS).
    When calculating the bin boundaries, if the last bin ends up with a width
   being less than 25%
    of the penultimate one, then the two are combined.

    @author Dickon Champion, STFC
    @date 25/02/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport Rebin : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Rebin"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rebins data with new X bin boundaries. For EventWorkspaces, you "
           "can very quickly rebin in-place by keeping the same output name "
           "and PreserveEvents=true.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Rebin"; }
  /// Algorithm's aliases
  const std::string alias() const override { return "rebin"; }
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override {
    return {"RebinToWorkspace", "Rebin2D",           "Rebunch",
            "Regroup",          "RebinByPulseTimes", "RebinByTimeAtSample"};
  }

  static std::vector<double>
  rebinParamsFromInput(const std::vector<double> &inParams,
                       const API::MatrixWorkspace &inputWS,
                       Kernel::Logger &logger);

protected:
  const std::string workspaceMethodName() const override { return "rebin"; }
  const std::string workspaceMethodOnTypes() const override {
    return "MatrixWorkspace";
  }
  const std::string workspaceMethodInputProperty() const override {
    return "InputWorkspace";
  }

  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void propagateMasks(API::MatrixWorkspace_const_sptr inputWS,
                      API::MatrixWorkspace_sptr outputWS, int hist);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_REBIN_H_*/
