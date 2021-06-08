// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

std::vector<std::string> MANTID_MDALGORITHMS_DLL parseDimensionNames(const std::string &names_string);

/** MaskMD : Mask an MDWorkspace. Can provide complex masking shapes over an
  exisitng MDWorkspace. Operates on a MDWorkspace in-situ.

  @date 2012-03-01
*/
class MANTID_MDALGORITHMS_DLL MaskMD : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Mask an MDWorkspace in-situ marking specified boxes as masked"; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"MaskDetectors"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
