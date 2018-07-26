#ifndef MANTID_ALGORITHMS_SORTXAXIS_H_
#define MANTID_ALGORITHMS_SORTXAXIS_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/**
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

/**
 * @brief SortXAxis will take Histogram or Point data and reorder it based on
 * the X Axis' values, whilst maintaining it's Dx, Y and E axis relative values.
 * @author Samuel Jones
 * @version 2.0
 * @date 07-2018
 * @copyright GNU General Public License
 */

class DLLExport SortXAxis : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::vector<std::size_t> createIndexes(const size_t);

  void sortIndicesByX(std::vector<std::size_t> &workspaceIndecies,
                      std::string order,
                      const Mantid::API::MatrixWorkspace *inputWorkspace,
                      unsigned int specNum);

  void
  copyYandEToOutputWorkspace(std::vector<std::size_t> &workspaceIndecies,
                             const Mantid::API::MatrixWorkspace *inputWorkspace,
                             Mantid::API::MatrixWorkspace_sptr outputWorkspace,
                             unsigned int SpecNum, bool isAProperHistogram);

  void copyXandDxToOutputWorkspace(
      std::vector<std::size_t> &workspaceIndecies,
      const Mantid::API::MatrixWorkspace *inputWorkspace,
      Mantid::API::MatrixWorkspace_sptr outputWorkspace, unsigned int specNum);

  void copyToOutputWorkspace(std::vector<std::size_t> &workspaceIndecies,
                             const Mantid::API::MatrixWorkspace *inputWorkspace,
                             Mantid::API::MatrixWorkspace_sptr outputWorkspace,
                             unsigned int specNum, bool isAProperHistogram);

  bool determineIfHistogramIsValid(
      const Mantid::API::MatrixWorkspace *inputWorkspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SORTXAXIS_H_ */
