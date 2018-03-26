#ifndef MANTID_ALGORITHMS_REBINRAGGED_H_
#define MANTID_ALGORITHMS_REBINRAGGED_H_

#include <map>
#include "MantidKernel/System.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {

/** ResampleX : TODO: DESCRIPTION

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
class DLLExport ResampleX : public Algorithms::Rebin {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"RebinToWorkspace", "Rebin2D",           "Rebunch",
            "Regroup",          "RebinByPulseTimes", "RebinByTimeAtSample"};
  }
  const std::string category() const override;
  const std::string alias() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Resample the x-axis of the data with the requested number of "
           "points.";
  }

  /// MADE PUBLIC FOR TESTING ONLY - DO NOT USE
  double determineBinning(MantidVec &xValues, const double xmin,
                          const double xmax);
  /// MADE PUBLIC FOR TESTING ONLY - DO NOT USE
  void setOptions(const int numBins, const bool useLogBins, const bool isDist);

private:
  const std::string workspaceMethodName() const override {
    return "";
  } // Override the one from Rebin to ignore us

  void init() override;
  void exec() override;

  std::map<std::string, std::string> validateInputs() override;
  bool m_useLogBinning = true;
  bool m_preserveEvents = true;
  int m_numBins = 0;
  bool m_isDistribution = false;
  bool m_isHistogram = true;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBINRAGGED_H_ */
