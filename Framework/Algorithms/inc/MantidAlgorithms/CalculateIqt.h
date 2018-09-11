#ifndef MANTID_ALGORITHMS_CALCULATEIQT_H_
#define MANTID_ALGORITHMS_CALCULATEIQT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {
/** Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at : <https://github.com/mantidproject/mantid>
    Code Documentation is available at : <http://doxygen.mantidproject.org>
*/

class DLLExport CalculateIqt : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  std::string rebinParamsAsString();
  API::MatrixWorkspace_sptr
  monteCarloErrorCalculation(API::MatrixWorkspace_sptr sample,
                             API::MatrixWorkspace_sptr resolution,
                             const std::string &rebinParams, const int seed,
                             bool calculateErrors, const int nIterations);

  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr workspace,
                                  const std::string &params);
  API::MatrixWorkspace_sptr integration(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  convertToPointData(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  extractFFTSpectrum(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr divide(API::MatrixWorkspace_sptr lhsWorkspace,
                                   API::MatrixWorkspace_sptr rhsWorkspace);
  API::MatrixWorkspace_sptr cropWorkspace(API::MatrixWorkspace_sptr workspace,
                                          double xMax);
  API::MatrixWorkspace_sptr
  replaceSpecialValues(API::MatrixWorkspace_sptr workspace);

  API::MatrixWorkspace_sptr
  removeInvalidData(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  normalizedFourierTransform(API::MatrixWorkspace_sptr workspace,
                             const std::string &rebinParams);
  API::MatrixWorkspace_sptr
  calculateIqt(API::MatrixWorkspace_sptr workspace,
               API::MatrixWorkspace_sptr resolutionWorkspace,
               const std::string &rebinParams);
  API::MatrixWorkspace_sptr doSimulation(API::MatrixWorkspace_sptr sample,
                                         API::MatrixWorkspace_sptr resolution,
                                         const std::string &rebinParams,
                                         const int seed);
  API::MatrixWorkspace_sptr setErrorsToStandardDeviation(
      const std::vector<API::MatrixWorkspace_sptr> &simulatedWorkspaces);
  API::MatrixWorkspace_sptr CalculateIqt::setErrorsToZero(
      const std::vector<API::MatrixWorkspace_sptr> &simulatedWorkspaces);
};

} // namespace Algorithms
} // namespace Mantid
#endif // MANTID_ALGORITHMS_MONTECARLOABSORPTION_H_
