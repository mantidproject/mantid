// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <stdexcept>

namespace Mantid {
namespace Beamline {
enum class ComponentType { Generic, Infinite, Grid, Rectangular, Structured, Unstructured, Detector, OutlineComposite };

static std::string componentTypeString(const ComponentType type) {
  switch (type) {
  case ComponentType::Generic:
    return "Generic";
  case ComponentType::Infinite:
    return "Infinite";
  case ComponentType::Grid:
    return "Grid";
  case ComponentType::Rectangular:
    return "Rectangular";
  case ComponentType::Structured:
    return "Structured";
  case ComponentType::Unstructured:
    return "Unstructured";
  case ComponentType::Detector:
    return "Detector";
  case ComponentType::OutlineComposite:
    return "OutlineComposite";
  default:
    throw std::invalid_argument("Unknown ComponentType type");
  }
}
} // namespace Beamline
} // namespace Mantid
