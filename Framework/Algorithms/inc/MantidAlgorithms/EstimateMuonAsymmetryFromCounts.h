#ifndef MANTID_ALGORITHM_ESTIMATEMUONASYMMETRYFROMCOUNTS_H_
#define MANTID_ALGORITHM_ESTIMATEMUONASYMMETRYFROMCOUNTS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {
/**Takes a muon workspace as input and estimates the asymmetry, using a simple
method.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Spectra - The spectra to be adjusted (by default all spectra are done)</LI>
<LI> StartX - The minimum time to include in the analysis </LI>
<LI> EndX - The maximum time to include in the analysis </LI>
</UL>


@author
@date 03/03/2017

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
class DLLExport EstimateMuonAsymmetryFromCounts : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override {
    return "EstimateMuonAsymmetryFromCounts";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm gives an estimate "
           "for the asymmetry.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ESTIMATEMUONASYMMETRYFROMCOUNTS_H_*/
