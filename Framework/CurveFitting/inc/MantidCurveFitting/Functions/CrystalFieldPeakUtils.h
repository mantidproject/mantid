// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>
#include <vector>

namespace Mantid {

namespace API {
class CompositeFunction;
class FunctionValues;
class IPeakFunction;
} // namespace API

namespace CurveFitting {
namespace Functions {
namespace CrystalFieldUtils {
/**
Utility functions to help set up peak functions in a Crystal Field spectrum.
*/
size_t buildSpectrumFunction(API::CompositeFunction &spectrum, const std::string &peakShape,
                             const API::FunctionValues &centresAndIntensities, const std::vector<double> &xVec,
                             const std::vector<double> &yVec, double fwhmVariation, double defaultFWHM,
                             size_t nRequiredPeaks, bool fixAllPeaks);
size_t updateSpectrumFunction(API::CompositeFunction &spectrum, const std::string &peakShape,
                              const API::FunctionValues &centresAndIntensities, size_t iFirst,
                              const std::vector<double> &xVec, const std::vector<double> &yVec, double fwhmVariation,
                              double defaultFWHM, bool fixAllPeaks);
size_t calculateNPeaks(const API::FunctionValues &centresAndIntensities);
size_t calculateMaxNPeaks(size_t nPeaks);

} // namespace CrystalFieldUtils
} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
