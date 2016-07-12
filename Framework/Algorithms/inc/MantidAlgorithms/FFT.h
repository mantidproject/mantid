#ifndef MANTID_ALGORITHM_FFT_H_
#define MANTID_ALGORITHM_FFT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {

/** Performs a Fast Fourier Transform of data

    @author Roman Tolchenov
    @date 07/07/2009

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport FFT : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FFT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs complex Fast Fourier Transform";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

protected:
  /// Perform validation of inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  /// Check whether supplied values are evenly spaced
  bool areBinWidthsUneven(const MantidVec &xValues) const;
  /// Get phase shift - user supplied or auto-calculated
  double getPhaseShift(const MantidVec &xValues);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FFT_H_*/
