#ifndef MANTID_ALGORITHMS_CREATEPSDBLEEDMASK_H_
#define MANTID_ALGORITHMS_CREATEPSDBLEEDMASK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorDiagnostic.h"

namespace Mantid {
namespace Algorithms {
/**

  This algorithm implements a "bleed" diagnostic for PSD detectors (i.e. long
  tube-based detectors).
  Required Properties:
  <UL>
  <LI> InputWorkspace  - The name of the input workspace. </LI>
  <LI> OutputMaskWorkspace - The name of the output workspace. Can be the same
  as the input one. </LI>
  <LI> MaxTubeRate - The maximum rate allowed for a tube. </LI>
  <LI> NIgnoredCentralPixels - The number of pixels about the centre to ignore.
  </LI>
  </UL>

  @author Martyn Gigg, Tessella plc
  @date 2011-01-10

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CreatePSDBleedMask : public DetectorDiagnostic {
public:
  /// Default constructor
  CreatePSDBleedMask();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CreatePSDBleedMask"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Runs a diagnostic test for saturation of PSD tubes and creates a "
           "MaskWorkspace marking the failed tube spectra.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"IdentifyNoisyDetectors"};
  }
  const std::string category() const override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Process a tube
  bool performBleedTest(const std::vector<int> &tubeIndices,
                        API::MatrixWorkspace_const_sptr inputWS, double maxRate,
                        int numIgnoredPixels);
  /// Mask a tube with the given workspace indices
  void maskTube(const std::vector<int> &tubeIndices,
                API::MatrixWorkspace_sptr workspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif // MANTID_ALGORITHMS_CREATEPSDBLEEDMASK_H_
