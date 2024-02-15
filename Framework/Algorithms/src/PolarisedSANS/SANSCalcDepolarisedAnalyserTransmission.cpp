// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarisedSANS/SANSCalcDepolarisedAnalyserTransmission.h"

namespace Mantid::Algorithms {

namespace {
namespace FitStartValues {
double constexpr T_E = 0.9;
double constexpr PXD = 12.6;
} // namespace FitStartValues
double constexpr LAMBDA_CONVERSION_FACTOR = 0.0733;
} // namespace

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(SANSCalcDepolarisedAnalyserTransmission)

void SANSCalcDepolarisedAnalyserTransmission::init() {}

void SANSCalcDepolarisedAnalyserTransmission::exec() {}

} // namespace Mantid::Algorithms
