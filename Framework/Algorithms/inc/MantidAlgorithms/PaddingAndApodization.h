#ifndef MANTID_ALGORITHM_PADDINGANDAPODUIZTION_H_
#define MANTID_ALGORITHM_PADDINGANDAPODUIZTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {
/**Takes a workspace as input and applies a
apodization function and/or padding

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Spectra - The spectra to be adjusted (by default all spectra are done)</LI>
<LI> ApodizationFunction - the apodization function to use </LI>
<LI> decayConstant - the decay constant for the apodization function</LI>
<LI> padding - the number of times to extend the data range by zeros </LI>
</UL>


@author Anthony Lim
@date 10/08/2017

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
class DLLExport PaddingAndApodization : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PaddingAndApodization"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm applies apodization and/or padding to input data.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  using fptr = double (*)(const double, const double);
  fptr getApodizationFunction(const std::string method);
  HistogramData::Histogram
  applyApodizationFunction(const HistogramData::Histogram &histogram,
                           const double decayConstant, fptr function);
  HistogramData::Histogram addPadding(const HistogramData::Histogram &histogram,
                                      const int padding);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_PADDINGANDAPODUIZTION_H_*/
