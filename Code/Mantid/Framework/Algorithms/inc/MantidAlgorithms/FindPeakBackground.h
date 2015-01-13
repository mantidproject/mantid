#ifndef MANTID_ALGORITHMS_FindPeakBackground_H_
#define MANTID_ALGORITHMS_FindPeakBackground_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** FindPeakBackground : Calculate Zscore for a Matrix Workspace

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport FindPeakBackground : public API::Algorithm {
public:
  FindPeakBackground();
  virtual ~FindPeakBackground();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FindPeakBackground"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Separates background from signal for spectra of a workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Utility"; }

private:
  std::string m_backgroundType; //< The type of background to fit

  /// Implement abstract Algorithm methods
  void init();
  /// Implement abstract Algorithm methods
  void exec();
  double moment4(MantidVec &X, size_t n, double mean);
  void estimateBackground(const MantidVec &X, const MantidVec &Y,
                          const size_t i_min, const size_t i_max,
                          const size_t p_min, const size_t p_max,
                          double &out_bg0, double &out_bg1, double &out_bg2);
  struct cont_peak {
    size_t start;
    size_t stop;
    double maxY;
  };
  struct by_len {
    bool operator()(cont_peak const &a, cont_peak const &b) {
      return a.maxY > b.maxY;
    }
  };
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FindPeakBackground_H_ */
