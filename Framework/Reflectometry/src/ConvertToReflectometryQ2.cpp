// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidReflectometry/ConvertToReflectometryQ2.h"

namespace Mantid::Reflectometry {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToReflectometryQ2)

int ConvertToReflectometryQ2::version() const { return 2; }
} // namespace Mantid::Reflectometry
