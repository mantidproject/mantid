// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_TRANSPOSEMD_H_
#define MANTID_MDALGORITHMS_TRANSPOSEMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/MDAxisValidator.h"
#include "MantidMDAlgorithms/DllConfig.h"
namespace Mantid {
namespace MDAlgorithms {

/** TransposeMD : Transpose an MDWorkspace. Allows dimensions to be collapsed
  down.
*/
class MANTID_MDALGORITHMS_DLL TransposeMD : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Transpose3D", "Transpose"};
  }
  const std::string category() const override;
  const std::string summary() const override;

  const std::string alias() const override { return "PermuteMD"; }

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_TRANSPOSEMD_H_ */
