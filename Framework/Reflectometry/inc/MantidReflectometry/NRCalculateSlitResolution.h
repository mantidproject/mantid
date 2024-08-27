// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidReflectometry/DllConfig.h"
#include <optional>

namespace Mantid {
namespace Reflectometry {

/** NRCalculateSlitResolution
 */

class MANTID_REFLECTOMETRY_DLL NRCalculateSlitResolution : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"CalculateSlits"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Reflectometry
} // namespace Mantid
