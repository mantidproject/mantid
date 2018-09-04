#ifndef MANTID_ALGORITHM_RRFMUON_H_
#define MANTID_ALGORITHM_RRFMUON_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ListValidator.h"
namespace Mantid {
namespace Algorithms {
/**Algorithm for calculating Muon decay envelope.

@author Raquel Alvarez, ISIS, RAL
@date 5/12/2014

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
class DLLExport RRFMuon : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RRFMuon"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Calculate Muon asymmetry in the rotating reference frame (RRF).";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateMuonAsymmetry"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Get conversion factor from frequency units to input workspace units
  double unitConversionFactor(std::string uin, std::string uuser);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_RRFMUON_H_*/