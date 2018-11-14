// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RESETNEGATIVES_H_
#define MANTID_ALGORITHMS_RESETNEGATIVES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ResetNegatives : Reset negative values to something else.

  @date 2012-02-01
*/
class DLLExport ResetNegatives : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Reset negative values to something else.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void pushMinimum(API::MatrixWorkspace_const_sptr minWS,
                   API::MatrixWorkspace_sptr wksp, API::Progress &prog);
  void changeNegatives(API::MatrixWorkspace_const_sptr minWS,
                       const double spectrumNegativeValues,
                       API::MatrixWorkspace_sptr wksp, API::Progress &prog);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RESETNEGATIVES_H_ */
