#ifndef MANTID_ALGORITHMS_WIENERSMOOTH_H_
#define MANTID_ALGORITHMS_WIENERSMOOTH_H_

#include "MantidAPI/Algorithm.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {

/** WienerSmooth algorithm performes smoothing data in a spectrum of a matrix
  workspace
  using the Wiener filter smoothing.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport WienerSmooth : public API::Algorithm {
public:
  const std::string name() const override { return "WienerSmooth"; }
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"FFTSmooth"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  std::pair<double, double>
  getStartEnd(const Mantid::HistogramData::HistogramX &X,
              bool isHistogram) const;
  API::MatrixWorkspace_sptr copyInput(API::MatrixWorkspace_sptr inputWS,
                                      size_t wsIndex);
  API::MatrixWorkspace_sptr
  smoothSingleSpectrum(API::MatrixWorkspace_sptr inputWS, size_t wsIndex);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_WIENERSMOOTH_H_ */
