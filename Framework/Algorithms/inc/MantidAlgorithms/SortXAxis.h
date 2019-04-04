// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SORTXAXIS_H_
#define MANTID_ALGORITHMS_SORTXAXIS_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

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
  std::vector<std::size_t> createIndices(const size_t);

  void sortIndicesByX(std::vector<std::size_t> &workspaceIndices,
                      std::string order,
                      const Mantid::API::MatrixWorkspace &inputWorkspace,
                      unsigned int specNum);

  void
  copyYandEToOutputWorkspace(std::vector<std::size_t> &workspaceIndices,
                             const Mantid::API::MatrixWorkspace &inputWorkspace,
                             Mantid::API::MatrixWorkspace &outputWorkspace,
                             unsigned int SpecNum);

  void copyXandDxToOutputWorkspace(
      std::vector<std::size_t> &workspaceIndices,
      const Mantid::API::MatrixWorkspace &inputWorkspace,
      Mantid::API::MatrixWorkspace &outputWorkspace, unsigned int specNum);

  void copyToOutputWorkspace(std::vector<std::size_t> &workspaceIndices,
                             const Mantid::API::MatrixWorkspace &inputWorkspace,
                             Mantid::API::MatrixWorkspace &outputWorkspace,
                             unsigned int specNum);

  void determineIfHistogramIsValid(
      const Mantid::API::MatrixWorkspace &inputWorkspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SORTXAXIS_H_ */
