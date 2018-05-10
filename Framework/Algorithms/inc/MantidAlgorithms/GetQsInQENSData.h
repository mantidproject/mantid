#ifndef MANTID_ALGORITHMS_GETQSINQENSDATA_H
#define MANTID_ALGORITHMS_GETQSINQENSDATA_H

#include "MantidAPI/Algorithm.h"

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** Extracts Q-values from a workspace containing QENS data

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to extract the Q-values from
   </LI>
    <LI> RaiseMode - If set to true, exceptions will be raised, instead of an
   empty list being returned </LI>
    <LI> Qvalues - The Q-values extracted from the input workspace </LI>
    </UL>

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport GetQsInQENSData : public API::Algorithm {
public:
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Inelastic\\Indirect"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string summary() const override {
    return "Get Q-values in the vertical axis of a MatrixWorkspace containing "
           "QENS S(Q,E) of S(theta,E) data.";
  }

  /// Algorithms name for identification. @see Algorithm::name
  const std::string name() const override { return "GetQsInQENSData"; }

  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialization code
  void init() override;

  /// Execution code
  void exec() override;

  /// Extracts Q-values from the specified matrix workspace
  MantidVec extractQValues(const Mantid::API::MatrixWorkspace_sptr workspace);
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GETQSINQENSDATA_H */