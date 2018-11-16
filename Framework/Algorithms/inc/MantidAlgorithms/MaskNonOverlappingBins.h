// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MASKNONOVERLAPPINGBINS_H_
#define MANTID_ALGORITHMS_MASKNONOVERLAPPINGBINS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** MaskNonOverlappingBins : Compares the X ranges of two workspace and
 * masks the non-overlapping bins in the first workspace.
 */
class MANTID_ALGORITHMS_DLL MaskNonOverlappingBins : public API::Algorithm {
public:
  std::string const name() const override;
  int version() const override;
  std::string const category() const override;
  std::string const summary() const override;
  std::vector<std::string> const seeAlso() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  void checkXSorting(API::MatrixWorkspace const &inputWS,
                     API::MatrixWorkspace const &comparisonWS);
  bool isCommonBins(API::MatrixWorkspace const &inputWS,
                    API::MatrixWorkspace const &comparisonWS);
  void processRagged(API::MatrixWorkspace const &inputWS,
                     API::MatrixWorkspace const &comparisonWS,
                     API::MatrixWorkspace &outputWS);
  void processNonRagged(API::MatrixWorkspace const &inputWS,
                        API::MatrixWorkspace const &comparisonWS,
                        API::MatrixWorkspace &outputWS);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MASKNONOVERLAPPINGBINS_H_ */
