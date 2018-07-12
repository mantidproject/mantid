#ifndef MANTID_ALGORITHM_REGROUP_H_
#define MANTID_ALGORITHM_REGROUP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
}
namespace Algorithms {
/** Takes a 2D workspace as input and regroups the data according to the input
   regroup parameters.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> RegroupParameters -  </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    @author Roman Tolchenov
    @date 16/07/2008

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
class DLLExport Regroup : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Regroup"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Regroups data with new bin boundaries.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Rebin"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Rebin"; }

  int newAxis(const std::vector<double> &params,
              const std::vector<double> &xold, std::vector<double> &xnew,
              std::vector<int> &xoldIndex);

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void rebin(const HistogramData::HistogramX &xold,
             const HistogramData::HistogramY &yold,
             const HistogramData::HistogramE &eold, std::vector<int> &xoldIndex,
             HistogramData::HistogramY &ynew, HistogramData::HistogramE &enew,
             bool distribution);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REGROUP_H_*/
