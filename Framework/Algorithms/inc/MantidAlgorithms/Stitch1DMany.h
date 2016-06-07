#ifndef MANTID_ALGORITHMS_STITCH1DMANY_H_
#define MANTID_ALGORITHMS_STITCH1DMANY_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** Stitch1DMany : Stitches multiple Matrix Workspaces together into a single
 output.

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
class DLLExport Stitch1DMany : public API::Algorithm {
public:
  /// Default constructor
  Stitch1DMany()
      : m_numWorkspaces(0), m_manualScaleFactor(1.0), m_scaleRHSWorkspace(true),
        m_useManualScaleFactor(false){};
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override { return "Stitch1DMany"; }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Stitches histogram matrix workspaces together";
  }
  /// Validates algorithm inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method.
  void exec() override;

  // Data
  std::vector<Mantid::API::Workspace_sptr> m_inputWorkspaces;
  std::vector<double> m_startOverlaps;
  std::vector<double> m_endOverlaps;
  std::vector<double> m_params;
  std::vector<double> m_scaleFactors;
  Mantid::API::Workspace_sptr m_outputWorkspace;

  size_t m_numWorkspaces;
  double m_manualScaleFactor;
  bool m_scaleRHSWorkspace;
  bool m_useManualScaleFactor;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_STITCH1DMANY_H_ */
