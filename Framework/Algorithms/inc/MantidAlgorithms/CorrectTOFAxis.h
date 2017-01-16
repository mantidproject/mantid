#ifndef MANTID_ALGORITHMS_CORRECTTOFAXIS_H_
#define MANTID_ALGORITHMS_CORRECTTOFAXIS_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <vector>

namespace Mantid {
namespace API {
class SpectrumInfo;
}

namespace Algorithms {

/** CorrectTOFAxis : Corrects the time-of-flight axis with regards to
  the incident energy and the L1+L2 distance or a reference workspace.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL CorrectTOFAxis : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  size_t m_elasticBinIndex = EMPTY_LONG();
  API::ITableWorkspace_const_sptr m_eppTable;
  API::MatrixWorkspace_const_sptr m_inputWs;
  API::MatrixWorkspace_const_sptr m_referenceWs;
  std::vector<size_t> m_workspaceIndices;

  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  void useReferenceWorkspace(API::MatrixWorkspace_sptr outputWs);
  void correctManually(API::MatrixWorkspace_sptr outputWs);
  double averageL2(const API::SpectrumInfo &spectrumInfo);
  void averageL2AndEPP(const API::SpectrumInfo &spectrumInfo, double &l2,
                       double &epp);
  std::vector<size_t> referenceWorkspaceIndices() const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CORRECTTOFAXIS_H_ */
