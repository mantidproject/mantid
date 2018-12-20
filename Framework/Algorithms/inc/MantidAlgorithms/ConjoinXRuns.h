#ifndef MANTID_ALGORITHMS_CONJOINXRUNS_H_
#define MANTID_ALGORITHMS_CONJOINXRUNS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

#include <list>

namespace Mantid {
namespace Algorithms {

/** ConjoinXRuns : This algorithms joins the input workspaces horizontally,
* i.e. by appending (concatenating) their columns.

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
class MANTID_ALGORITHMS_DLL ConjoinXRuns : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

protected:
  void fillHistory() override;

private:
  void init() override;
  void exec() override;

  std::string checkLogEntry(API::MatrixWorkspace_sptr) const;
  std::vector<double> getXAxis(API::MatrixWorkspace_sptr) const;
  void joinSpectrum(int64_t);

  /// Sample log entry name
  std::string m_logEntry;
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress;
  /// List of input matrix workspaces
  std::list<API::MatrixWorkspace_sptr> m_inputWS;
  /// Output workspace
  API::MatrixWorkspace_sptr m_outWS;
  /// X-axis cache if sample log is given
  std::map<std::string, std::vector<double>> m_axisCache;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONJOINXRUNS_H */
