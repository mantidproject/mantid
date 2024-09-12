// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
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

/** CalculateSlits
 */

class MANTID_REFLECTOMETRY_DLL CalculateSlits : public API::DataProcessorAlgorithm {
public:
  CalculateSlits();
  ~CalculateSlits() override;

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"NRCalculateSlitResolution"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Reflectometry
} // namespace Mantid
