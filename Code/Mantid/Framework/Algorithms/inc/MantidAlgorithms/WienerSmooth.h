#ifndef MANTID_ALGORITHMS_WIENERSMOOTH_H_
#define MANTID_ALGORITHMS_WIENERSMOOTH_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

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
  WienerSmooth();
  virtual ~WienerSmooth();

  virtual const std::string name() const { return "WienerSmooth"; }
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();

  std::pair<double, double> getStartEnd(const MantidVec &X,
                                        bool isHistogram) const;
  API::MatrixWorkspace_sptr copyInput(API::MatrixWorkspace_sptr inputWS,
                                      size_t wsIndex);
  API::MatrixWorkspace_sptr
  smoothSingleSpectrum(API::MatrixWorkspace_sptr inputWS, size_t wsIndex);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_WIENERSMOOTH_H_ */