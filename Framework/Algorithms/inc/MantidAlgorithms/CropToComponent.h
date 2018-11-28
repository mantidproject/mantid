// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CROPTOCOMPONENT_H_
#define MANTID_ALGORITHMS_CROPTOCOMPONENT_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CropToComponent : Crops a workspace to a set of components.
 */
class MANTID_ALGORITHMS_DLL CropToComponent final
    : public API::DistributedAlgorithm {
public:
  const std::string name() const override final;
  int version() const override final;
  const std::vector<std::string> seeAlso() const override {
    return {"CropWorkspace"};
  }
  const std::string category() const override final;
  const std::string summary() const override final;

protected:
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override final;
  void exec() override final;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CROPTOCOMPONENT_H_ */
