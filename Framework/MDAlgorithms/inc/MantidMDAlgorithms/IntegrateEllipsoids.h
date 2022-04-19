// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid::MDAlgorithms {

// Detect and pass to the correct version
class DLLExport IntegrateEllipsoids final : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Integrate Single Crystal Diffraction Bragg peaks using 3D "
           "ellipsoids.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"IntegrateEllipsoidsTwoStep"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Mantid::MDAlgorithms
