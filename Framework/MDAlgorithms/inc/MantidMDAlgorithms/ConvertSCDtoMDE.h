// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVERTWANDSCDTOMDE_H_
#define MANTID_MDALGORITHMS_CONVERTWANDSCDTOMDE_H_

#include "MantidMDAlgorithms/BoxControllerSettingsAlgorithm.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertSCDtoMDE : TODO: DESCRIPTION
*/
class MANTID_MDALGORITHMS_DLL ConvertSCDtoMDE
    : public BoxControllerSettingsAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTWANDSCDTOMDE_H_ */
