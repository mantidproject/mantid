// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_THRESHOLDMD_H_
#define MANTID_MDALGORITHMS_THRESHOLDMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** ThresholdMD : TODO: DESCRIPTION
*/
class DLLExport ThresholdMD : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Threshold an MDHistoWorkspace.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SmoothMD"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_THRESHOLDMD_H_ */