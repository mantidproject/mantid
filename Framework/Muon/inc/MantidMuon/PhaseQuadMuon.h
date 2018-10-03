#ifndef MANTID_ALGORITHM_PHASEQUADMUON_H_
#define MANTID_ALGORITHM_PHASEQUADMUON_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/**Algorithm for calculating Muon spectra.

@author Raquel Alvarez, ISIS, RAL
@date 1/12/2014

Copyright &copy; 2014-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
National Laboratory

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
class DLLExport PhaseQuadMuon : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PhaseQuad"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Generates a quadrature phase signal.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonMaxent"};
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;
  /// Calculate the normalization constants
  std::vector<double> getExponentialDecay(const API::MatrixWorkspace_sptr &ws);
  /// Create squashograms
  API::MatrixWorkspace_sptr squash(const API::MatrixWorkspace_sptr &ws,
                                   const API::ITableWorkspace_sptr &phase,
                                   const std::vector<double> &n0);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PHASEQUAD_H_*/
