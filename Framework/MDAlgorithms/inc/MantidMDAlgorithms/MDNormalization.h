// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_MDNORMALIZATION_H_
#define MANTID_MDALGORITHMS_MDNORMALIZATION_H_

#include "MantidMDAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace MDAlgorithms {

/** MDNormalization : TODO: DESCRIPTION
*/
class MANTID_MDALGORITHMS_DLL MDNormalization : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CropWorkspaceForMDNorm", "MDNormSCD", "MDNormDirectSC", "RecalculateTrajectoriesExtents"};
  }
private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override final;
  std::string QDimensionName(std::vector<double> projection);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MDNORMALIZATION_H_ */
