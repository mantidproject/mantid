#ifndef MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_
#define MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ApplyDeadTimeCorr : The dead-time is here applied according to the
  non-extendable paralyzable dead time model.

  @author Robert Whitley, RAL
  @date 2011-11-30

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
class DLLExport ApplyDeadTimeCorr : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ApplyDeadTimeCorr"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Apply deadtime correction to each spectrum of a workspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"CalMuonDeadTime"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Muon;CorrectionFunctions\\EfficiencyCorrections";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_APPLYDEADTIMECORR_H_ */
