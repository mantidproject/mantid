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
} // namespace Beamline
} // namespace Mantid
