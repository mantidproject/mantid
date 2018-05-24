#ifndef MANTID_ALGORITHMS_STRIPVANADIUMPEAKS2_H_
#define MANTID_ALGORITHMS_STRIPVANADIUMPEAKS2_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ParallelAlgorithm.h"

namespace Mantid {
namespace Algorithms {

/** StripVanadiumPeaks2

  @author Wenduo Zhou
  @date 2011-10-07

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
class DLLExport StripVanadiumPeaks2 : public API::ParallelAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "StripVanadiumPeaks"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"FindPeaks", "StripPeaks"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\PeakCorrections;Optimization\\PeakFinding;"
           "Diffraction\\Corrections";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm removes peaks (at vanadium d-spacing positions by "
           "default)"
           " out of a background by linearly/quadratically interpolating over "
           "the expected peak positions. ";
  }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_STRIPVANADIUMPEAKS2_H_ */
