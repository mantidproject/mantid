// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_AVERAGESPECTRUMBACKGROUND_H_
#define MANTID_ALGORITHMS_AVERAGESPECTRUMBACKGROUND_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {

/** AverageSpectrumBackground : TODO: DESCRIPTION
 */
class DLLExport AverageSpectrumBackground : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  // Sums spectra bin by bin in the given range using the child algorithm
  // GroupDetectors.
  API::MatrixWorkspace_sptr AverageSpectrumBackground::groupBackgroundDetectors(
      API::MatrixWorkspace_sptr inputWS, const std::vector<size_t> indexList);

  std::vector<size_t> AverageSpectrumBackground::getSpectraFromRange(
      const std::vector<size_t> range);

  /** Overridden Algorithm methods **/

  // Initialize the algorithm
  void init() override;
  // Execute the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_AVERAGESPECTRUMBACKGROUND_H_ */