// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMD_H_
#define MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** FlippingRatioCorrectionMD : Algorithm to correct
 *  MDEvents for flipping ratio
 */
class MANTID_MDALGORITHMS_DLL FlippingRatioCorrectionMD
    : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  template <typename MDE, size_t nd>
  void executeTemplatedMDE(
      typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
  std::vector<double> m_factor;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMD_H_ */
