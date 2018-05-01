#ifndef MANTID_ALGORITHMS_RUNCOMBINATIONHELPER_H_
#define MANTID_ALGORITHMS_RUNCOMBINATIONHELPER_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/Logger.h"

#include <list>
#include <vector>

namespace Mantid {
namespace Algorithms {

/** RunCombinationHelper : This holds some useful utilities for operations
 * involving transformations of lists of workspaces into single one.
 * E.g. this is used commonly between MergeRuns and JoinRuns

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

namespace RunCombinationOptions {
static const std::string SKIP_BEHAVIOUR = "Skip File";
static const std::string STOP_BEHAVIOUR = "Stop";
static const std::string REBIN_BEHAVIOUR = "Rebin";
static const std::string FAIL_BEHAVIOUR = "Fail";
}

class MANTID_ALGORITHMS_DLL RunCombinationHelper {
public:
  std::string checkCompatibility(API::MatrixWorkspace_sptr,
                                 bool checkNumberHistograms = false);
  void setReferenceProperties(API::MatrixWorkspace_sptr);
  static std::vector<std::string>
  unWrapGroups(const std::vector<std::string> &);
  std::list<API::MatrixWorkspace_sptr>
  validateInputWorkspaces(const std::vector<std::string> &inputWorkspaces,
                          Kernel::Logger &g_log);

private:
  size_t m_numberSpectra;
  size_t m_numberDetectors;
  std::string m_xUnit;
  std::string m_yUnit;
  std::string m_spectrumAxisUnit;
  std::string m_instrumentName;
  bool m_isHistogramData;
  bool m_isScanning;
  std::vector<bool> m_hasDx;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RUNCOMBINATIONHELPER_H_ */
