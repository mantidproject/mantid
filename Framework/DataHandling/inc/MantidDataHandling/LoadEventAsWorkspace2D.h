// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/** LoadEventAsWorkspace2D : Load event data, integrating the events during loading
 */
class MANTID_DATAHANDLING_DLL LoadEventAsWorkspace2D : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadEventNexus", "HFIRSANS2Wavelength"}; }

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace DataHandling
} // namespace Mantid
