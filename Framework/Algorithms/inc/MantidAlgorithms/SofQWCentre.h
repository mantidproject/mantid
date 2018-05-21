#ifndef MANTID_ALGORITHMS_SOFQWCENTRE_H_
#define MANTID_ALGORITHMS_SOFQWCENTRE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/SofQCommon.h"

namespace Mantid {
namespace Algorithms {
/** Converts a 2D workspace that has axes of energy transfer against spectrum
number to
one that gives intensity as a function of momentum transfer against energy.

Required Properties:
<UL>
<LI> InputWorkspace  - Reduced data in units of energy transfer. Must have
common bins. </LI>
<LI> OutputWorkspace - The name to use for the q-w workspace. </LI>
<LI> QAxisBinning    - The bin parameters to use for the q axis. </LI>
<LI> Emode           - The energy mode (direct or indirect geometry). </LI>
<LI> Efixed          - Value of fixed energy: EI (emode=1) or EF (emode=2)
(meV). </LI>
</UL>

@author Russell Taylor, Tessella plc
@date 24/02/2010

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
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SofQWCentre : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SofQWCentre"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts a 2D workspace that has axes in *units* of **DeltaE** "
           "(energy transfer) against spectrum number to one "
           "that gives intensity as a function of **DeltaE** against "
           "**momentum transfer** ";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SofQW", "Rebin2D"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\SofQW"; }

private:
  /// Convert the workspace to a distribution
  static void makeDistribution(API::MatrixWorkspace &outputWS,
                               const std::vector<double> &qAxis);
  /// Initialization code
  void init() override;
  /// Execution code
  void exec() override;

  SofQCommon m_EmodeProperties;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SOFQWCENTRE_H_*/
