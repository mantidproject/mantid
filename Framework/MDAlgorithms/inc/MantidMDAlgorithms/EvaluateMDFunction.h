// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_EVALUATEMDFUNCTION_H_
#define MANTID_MDALGORITHMS_EVALUATEMDFUNCTION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** EvaluateMDFunction : TODO: DESCRIPTION
 */
class DLLExport EvaluateMDFunction : public API::Algorithm,
                                     public API::DeprecatedAlgorithm {
public:
  EvaluateMDFunction();

  const std::string name() const override { return "EvaluateMDFunction"; }
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateMDWorkspace", "FakeMDEventData"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_EVALUATEMDFUNCTION_H_ */
