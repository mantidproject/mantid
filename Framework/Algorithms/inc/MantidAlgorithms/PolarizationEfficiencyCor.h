#ifndef MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_
#define MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

namespace Mantid {
namespace Algorithms {

/** PolarizationEfficiencyCor: a generalised polarization correction
  algorithm. Depending on the value of property "CorrectionMethod" it
  calls either PolarizationCorrectionFredrikze or PolarizationCorrectionWildes
  inetrnally.

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL PolarizationEfficiencyCor : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PolarizationCorrectionFredrikze"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void execWildes();
  void execFredrikze();

  void checkWorkspaces() const;
  void checkWildesProperties() const;
  void checkFredrikzeProperties() const;

  std::vector<std::string> getWorkspaceNameList() const;
  API::WorkspaceGroup_sptr getWorkspaceGroup() const;
  API::MatrixWorkspace_sptr getEfficiencies();
  bool needInterpolation(API::MatrixWorkspace const &efficiencies,
                         API::MatrixWorkspace const &inWS) const;
  API::MatrixWorkspace_sptr
  convertToHistogram(API::MatrixWorkspace_sptr efficiencies);
  API::MatrixWorkspace_sptr interpolate(API::MatrixWorkspace_sptr efficiencies,
                                        API::MatrixWorkspace_sptr inWS);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_ */
